#include "Mqtt.hpp"
#include "Helper.hpp"
#include "Cache.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>

#define BIT_STOP (1 << 0)

using namespace ServicePayload;
using namespace ServiceType;

static const char *TAG = "Mqtt";

static TaskHandle_t _taskRecieveMsg = nullptr;
static TaskHandle_t _taskSendMsg = nullptr;
static QueueHandle_t _queueReciveMsg = nullptr;
static QueueHandle_t _queueSendMsg = nullptr;
static EventGroupHandle_t _eventGroupStop = nullptr;

static esp_mqtt_client_handle_t _gb_client = nullptr;
static esp_mqtt_event_id_t _gb_state_mqtt = MQTT_EVENT_DISCONNECTED;

static std::map<ChannelMqtt, std::string> _gb_channel_topic = {
    {ChannelMqtt::CHANNEL_CONFIG, "/config"},
    {ChannelMqtt::CHANNEL_DATA, "/data"},
    {ChannelMqtt::CHANNEL_CONTROL, "/control"},
    {ChannelMqtt::CHANNEL_NOTIFY, "/notify"},
    {ChannelMqtt::CHANNEL_SENSOR, "/sensor"}};

void Mqtt::mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);

  Mqtt *self = static_cast<Mqtt *>(handler_args);

  int msg_id;
  esp_mqtt_event_handle_t event = (esp_mqtt_event_t *)event_data;
  esp_mqtt_client_handle_t client = event->client;
  ESP_LOGW(TAG, "TCP log \n - connect_return_code: %d\n - error_type: %d\n - esp_transport_sock_errno: %d", event->error_handle->connect_return_code, event->error_handle->error_type, event->error_handle->esp_transport_sock_errno);

  switch (event_id)
  {
  case MQTT_EVENT_CONNECTED:
  {
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    self->_isConnected = true;
    _gb_state_mqtt = MQTT_EVENT_CONNECTED;
    break;
  }
  case MQTT_EVENT_DISCONNECTED:
  {
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    self->_isConnected = false;
    _gb_state_mqtt = MQTT_EVENT_DISCONNECTED;
    break;
  }
  case MQTT_EVENT_SUBSCRIBED:
  {
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  }

  case MQTT_EVENT_UNSUBSCRIBED:
  {
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  }

  case MQTT_EVENT_PUBLISHED:
  {
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    break;
  }

  case MQTT_EVENT_DATA:
  {
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");

    /* copy event */
    esp_mqtt_event_handle_t event_copy = (esp_mqtt_event_t *)malloc(sizeof(esp_mqtt_event_t));

    if (event_copy == NULL)
    {
      ESP_LOGE(TAG, "Memory allocation failed");
      break;
    }

    memcpy(event_copy, event, sizeof(esp_mqtt_event_t));

    if (xQueueSend(_queueReciveMsg, &event_copy, 0) != pdPASS)
    {
      ESP_LOGE(TAG, "Queue send failed");
      free(event_copy);

      /* clear queue */
      xQueueReset(_queueReciveMsg);
    }

    break;
  }
  case MQTT_EVENT_ERROR:
  {
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    break;
  }
  default:
    ESP_LOGI(TAG, "Other event id:%d", event->event_id);
    break;
  }
}

int Mqtt::sendMessage(ChannelMqtt chanel, std::string msg)
{
  ESP_LOGI(TAG, "Send message at topic: \"%s\"", _gb_channel_topic[chanel].c_str());
  if (_gb_client != nullptr && _gb_state_mqtt == MQTT_EVENT_CONNECTED)
  {
    return esp_mqtt_client_publish(_gb_client, _gb_channel_topic[chanel].c_str(), msg.c_str(), 0, 1, 0);
  }
  return -1;
}

bool Mqtt::isStarted(void)
{
  return this->_isStarted;
}

void Mqtt::recieveMsg(void *arg)
{
  Mqtt *self = static_cast<Mqtt *>(arg);
  esp_mqtt_event_handle_t evt;

  while (true)
  {
    /* disbale: _gb_state_mqtt != MQTT_EVENT_CONNECTED */
    if (_queueReciveMsg == NULL || _gb_client == nullptr)
    {
      ESP_LOGW(TAG, "Queue recieve msg is null or client is null");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    if (xQueueReceive(_queueReciveMsg, &evt, portMAX_DELAY) == pdPASS)
    {
      std::string topic(evt->topic, evt->topic_len);
      std::string payload(evt->data, evt->data_len);

      ESP_LOGI(TAG, "Topic: %s", topic.c_str());
      ESP_LOGI(TAG, "Payload: %s", payload.c_str());

      try
      {
        /* handle message from here */
      }
      catch (const std::exception &e)
      {
        std::cerr << e.what() << '\n';
      }

      free(evt); // release memory
    }
  }
}

void Mqtt::start(void)
{
  if (this->_isStarted)
  {
    ESP_LOGW(TAG, "Mqtt is started");
    return;
  }
  ESP_LOGI(TAG, "Mqtt start");
  xTaskCreate(&Mqtt::init, "init::Mqtt", 4 * 1024, this, 6, NULL);
}

void Mqtt::stop(void)
{
  if (!this->_isStarted)
  {
    ESP_LOGW(TAG, "Mqtt is stopped");
    return;
  }

  _eventGroupStop = (_eventGroupStop == nullptr) ? xEventGroupCreate() : _eventGroupStop;

  ESP_LOGI(TAG, "Mqtt stop");
  xTaskCreate(&Mqtt::deinit, "deinit::Mqtt", 4 * 1024, this, 6, NULL);

  /* wait task done */
  xEventGroupWaitBits(_eventGroupStop, BIT_STOP, pdTRUE, pdFALSE, pdMS_TO_TICKS(5000)); // just 5s
}

void Mqtt::init(void *arg)
{
  Mqtt *self = static_cast<Mqtt *>(arg);

  if (cacheManager.m_username.length() == 0 || cacheManager.m_password.length() == 0)
  {
    ESP_LOGE(TAG, "Username or password is empty.");
    vTaskDelete(NULL);
  }

  std::string url = std::string("mqtt://") + std::string(CONFIG_HOST_SERVER) + std::string(":") + std::to_string(CONFIG_HOST_MQTT_PORT);
  std::string clientId = helperString::genClientId();

  esp_mqtt_client_config_t mqtt_cfg = {
      .broker{
          .address = {
              .uri = url.c_str(),
          },
      },
      .credentials{.username = cacheManager.m_username.c_str(), .client_id = clientId.c_str(), .authentication = {
                                                                                                   .password = cacheManager.m_password.c_str(),
                                                                                               }},
      .task{
          .priority = 5,
          .stack_size = 8 * 1024,
      }};

  ESP_LOGI(TAG, "MQTT ClientId: %s", mqtt_cfg.credentials.client_id);

  esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, self->mqtt_event_handler, self);
  esp_mqtt_client_start(client);
  _gb_client = client;

  if (_queueReciveMsg == nullptr)
  {
    _queueReciveMsg = xQueueCreate(10, sizeof(esp_mqtt_event_handle_t));
  }

  if (_taskRecieveMsg == nullptr)
  {
    xTaskCreate(&self->recieveMsg, "recieveMsg", 8 * 1024, self, 5, &_taskRecieveMsg);
  }

  self->_isStarted = true;
  vTaskDelete(NULL);
}

void Mqtt::deinit(void *arg)
{
  Mqtt *self = static_cast<Mqtt *>(arg);
  /* handle deinit Mqtt */

  if (_gb_client != nullptr)
  {
    /* step: ungister event mqtt */
    esp_mqtt_client_unregister_event(_gb_client, MQTT_EVENT_ANY, self->mqtt_event_handler);

    /* step: unsub toppic */
    for (auto &topic : _gb_channel_topic)
    {
      if (topic.second.length() > 0)
      {
        esp_mqtt_client_unsubscribe(_gb_client, topic.second.c_str());
      }
    }

    /* step: stop recieve msg */
    if (_taskRecieveMsg != nullptr)
    {
      vTaskDelete(_taskRecieveMsg);
      _taskRecieveMsg = nullptr;
    }

    /* step: disconnect mqtt */
    esp_mqtt_client_disconnect(_gb_client);

    /* step: stop mqtt */
    esp_mqtt_client_stop(_gb_client);

    /* step: destroy mqtt client */
    esp_err_t err = esp_mqtt_client_destroy(_gb_client);

    ESP_LOGI(TAG, "Destroy mqtt client: %s", esp_err_to_name(err));
    _gb_client = nullptr;
    _gb_state_mqtt = MQTT_EVENT_DISCONNECTED;
  }

  /* notify event stop */
  if (_eventGroupStop)
  {
    xEventGroupSetBits(_eventGroupStop, BIT_STOP);
  }

  self->_isStarted = false;
  vTaskDelete(NULL);
}

void Mqtt::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "Mqtt onRecive from %d", s);
  switch (s)
  {
  case CentralServices::MESH:
  {
    RecievePayload_2<MqttEventType, nullptr_t> *payload = static_cast<RecievePayload_2<MqttEventType, nullptr_t> *>(data);
    if (payload->type == MqttEventType::EVENT_MQTT_START)
    {
      /* start mesh service */
      this->start();
    }
    delete payload;
    break;
  }
  case CentralServices::API_CALLER:
  {
    RecievePayload_2<MqttEventType, nullptr_t> *payload = static_cast<RecievePayload_2<MqttEventType, nullptr_t> *>(data);
    if (payload->type == MqttEventType::EVENT_MQTT_START)
    {
      /* start mesh service */
      this->start();
    }
    delete payload;
    break;
  }
  case CentralServices::SENSOR:
  {
    RecievePayload_2<SensorType, std::string> *payload = static_cast<RecievePayload_2<SensorType, std::string> *>(data);
    if (payload->type == SENSOR_SEND_SAMPLE_DATA)
    {
      this->sendMessage(ChannelMqtt::CHANNEL_SENSOR, payload->data);
    }
    delete payload;
    break;
  }

  default:
    break;
  }
}