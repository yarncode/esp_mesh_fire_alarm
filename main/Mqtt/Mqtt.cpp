#include "Mqtt.hpp"
#include "ApiCaller.hpp"
#include "Cache.h"
#include "Helper.hpp"
#include "Relay.hpp"

#include <chrono>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#define BIT_STOP (1 << 0)

using namespace ServicePayload;
using namespace ServiceType;
using namespace nlohmann;

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
    {ChannelMqtt::CHANNEL_SENSOR, "/sensor"},
    {ChannelMqtt::CHANNEL_ACTIVE, "/active"},
    {ChannelMqtt::CHANNEL_IO_INPUT, "/input-io"},
    {ChannelMqtt::CHANNEL_IO_OUTPUT, "/output-io"},
};

bool Mqtt::isConnected(void) {
  return _gb_state_mqtt == MQTT_EVENT_CONNECTED ? true : false;
}

void Mqtt::notifySyncThresholdTrigger(void *arg) {
  Mqtt *self = static_cast<Mqtt *>(arg);
  json body;

  body["timestamp"] =
      std::chrono::system_clock::now().time_since_epoch().count();
  body["_type"] = common::CONFIG_NOTIFY_SYNC_THRESHOLD;
  body["smoke"] = cacheManager.thresholds_smoke;
  body["temp"] = cacheManager.thresholds_temp;
  body["humi"] = cacheManager.thresholds_humi;

  self->sendMessage(ChannelMqtt::CHANNEL_NOTIFY, body.dump());

  vTaskDeleteWithCaps(NULL);
}

void Mqtt::notifySyncGpio(void *arg) {
  Mqtt *self = static_cast<Mqtt *>(arg);
  json body;

  body["timestamp"] =
      std::chrono::system_clock::now().time_since_epoch().count();
  body["_type"] = common::CONFIG_NOTIFY_SYNC_GPIO;

  for (int i = 0; i < common::CONFIG_GPIO_INPUT_COUNT; i++) {
    body["input"][i] = cacheManager.input_state[i];
  }
  for (int i = 0; i < common::CONFIG_GPIO_OUTPUT_COUNT; i++) {
    body["output"][i] = cacheManager.output_state[i];
  }

  self->sendMessage(ChannelMqtt::CHANNEL_NOTIFY, body.dump());

  vTaskDeleteWithCaps(NULL);
}

void Mqtt::notifyDeviceCreated(void *arg) {
  Mqtt *self = static_cast<Mqtt *>(arg);
  json body;

  body["timestamp"] =
      std::chrono::system_clock::now().time_since_epoch().count();
  body["mac"] = chipInfo::getMacBleDevice();
  body["ip"] = cacheManager.ip;
  body["netmask"] = cacheManager.netmask;
  body["gateway"] = cacheManager.gateway;
  body["ssid"] = cacheManager.ssid;
  body["password"] = cacheManager.password;

  body["input_gpio_count"] = common::CONFIG_GPIO_INPUT_COUNT;
  body["output_gpio_count"] = common::CONFIG_GPIO_OUTPUT_COUNT;

  self->sendMessage(ChannelMqtt::CHANNEL_ACTIVE, body.dump());

  vTaskDeleteWithCaps(NULL);
}

void Mqtt::onConnected(void *handler_args, esp_event_base_t base,
                       int32_t event_id, void *event_data) {
  ESP_LOGI(TAG, "Mqtt Connected");
  Mqtt *self = static_cast<Mqtt *>(handler_args);

  /* subscribe topic */
  if (_gb_client != nullptr) {
    esp_mqtt_client_subscribe(
        _gb_client, _gb_channel_topic.at(ChannelMqtt::CHANNEL_CONTROL).c_str(),
        1);
    // esp_mqtt_client_subscribe(
    //     _gb_client, _gb_channel_topic.at(ChannelMqtt::CHANNEL_NOTIFY).c_str(),
    //     1);
  }

  /* check whether "api create device" is call recently */
  ApiCaller *api =
      static_cast<ApiCaller *>(self->getObserver(CentralServices::API_CALLER));
  if (api->getCacheApiCallRecently() == true) {
    api->setCacheApiCallRecently(false);
  }

  // xTaskCreateWithCaps(&Mqtt::notifySyncThresholdTrigger,
  // "notifySyncThresholdTrigger", 4 * 1024, self, 6, NULL, MALLOC_CAP_SPIRAM);
  // xTaskCreateWithCaps(&Mqtt::notifyDeviceCreated, "notifyDeviceCreated",
  //                     4 * 1024, self, 5, NULL, MALLOC_CAP_SPIRAM);
  xTaskCreateWithCaps(&Mqtt::notifySyncGpio, "notifySyncGpio", 4 * 1024, self,
                      7, NULL, MALLOC_CAP_SPIRAM);
}

void Mqtt::onDisconnected(void *handler_args, esp_event_base_t base,
                          int32_t event_id, void *event_data) {
  ESP_LOGE(TAG, "Mqtt Disconnected");
}

void Mqtt::mqtt_event_handler(void *handler_args, esp_event_base_t base,
                              int32_t event_id, void *event_data) {
  // ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32
  // "", base, event_id);

  Mqtt *self = static_cast<Mqtt *>(handler_args);

  int msg_id;
  esp_mqtt_event_handle_t event = (esp_mqtt_event_t *)event_data;
  esp_mqtt_client_handle_t client = event->client;
  // ESP_LOGW(TAG, "TCP log \n - connect_return_code: %d\n - error_type: %d\n -
  // esp_transport_sock_errno: %d", event->error_handle->connect_return_code,
  // event->error_handle->error_type,
  // event->error_handle->esp_transport_sock_errno);

  switch (event_id) {
  case MQTT_EVENT_CONNECTED: {
    // ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    self->_isConnected = true;
    _gb_state_mqtt = MQTT_EVENT_CONNECTED;
    self->onConnected(self, base, event_id, event_data);
    break;
  }
  case MQTT_EVENT_DISCONNECTED: {
    // ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    self->_isConnected = false;
    _gb_state_mqtt = MQTT_EVENT_DISCONNECTED;
    self->onDisconnected(self, base, event_id, event_data);
    break;
  }
  case MQTT_EVENT_SUBSCRIBED: {
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  }

  case MQTT_EVENT_UNSUBSCRIBED: {
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  }

  case MQTT_EVENT_PUBLISHED: {
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    break;
  }

  case MQTT_EVENT_DATA: {
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");

    /* copy event */
    esp_mqtt_event_handle_t event_copy =
        (esp_mqtt_event_t *)malloc(sizeof(esp_mqtt_event_t));

    if (event_copy == NULL) {
      ESP_LOGE(TAG, "Memory allocation failed");
      break;
    }

    memcpy(event_copy, event, sizeof(esp_mqtt_event_t));

    if (xQueueSend(_queueReciveMsg, &event_copy, 0) != pdPASS) {
      ESP_LOGE(TAG, "Queue send failed");
      free(event_copy);

      /* clear queue */
      xQueueReset(_queueReciveMsg);
    }

    break;
  }
  case MQTT_EVENT_ERROR: {
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    break;
  }
  default:
    ESP_LOGI(TAG, "Other event id:%d", event->event_id);
    break;
  }
}

void Mqtt::notifyAckPayload(std::string ack) {
  json _payload;
  _payload["timestamp"] =
      std::chrono::system_clock::now().time_since_epoch().count();
  _payload["ack"] = ack;
  _payload["code"] = common::CONFIG_NOTIFY_ACK_PAYLOAD;
  this->sendMessage(ChannelMqtt::CHANNEL_NOTIFY, _payload.dump());
}

int Mqtt::sendMessage(ChannelMqtt chanel, std::string msg) {
  if (_gb_client != nullptr && _gb_state_mqtt == MQTT_EVENT_CONNECTED) {
    ESP_LOGI(TAG, "Send message at topic: \"%s\"",
             _gb_channel_topic[chanel].c_str());
    return esp_mqtt_client_publish(
        _gb_client, _gb_channel_topic[chanel].c_str(), msg.c_str(), 0, 1, 0);
  }
  return -1;
}

bool Mqtt::isStarted(void) { return this->_isStarted; }

void Mqtt::recieveMsg(void *arg) {
  Mqtt *self = static_cast<Mqtt *>(arg);
  esp_mqtt_event_handle_t evt;

  while (true) {
    /* disbale: _gb_state_mqtt != MQTT_EVENT_CONNECTED */
    if (_queueReciveMsg == NULL || _gb_client == nullptr) {
      ESP_LOGW(TAG, "Queue recieve msg is null or client is null");
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    if (xQueueReceive(_queueReciveMsg, &evt, portMAX_DELAY) == pdPASS) {
      std::string topic(evt->topic, evt->topic_len);
      std::string payload(evt->data, evt->data_len);

      ESP_LOGI(TAG, "Topic: %s", topic.c_str());
      ESP_LOGI(TAG, "Payload: %s", payload.c_str());

      try {
        json data = json::parse(payload);

        /* handle message from topic control */
        if (_gb_channel_topic[ChannelMqtt::CHANNEL_CONTROL].compare(topic) ==
            0) {
          std::string _type = data.at("type").get<std::string>();
          std::string _ack = data.at("_ack").get<std::string>();

          /*
            {
              "pos": 1,
              "state": true,
              "_mode": "all" | "multiple" | "single",
              "_type": "gpio",
            }
          */

          if (_type.compare(common::CONFIG_KEY_OUTPUT) == 0) {
            std::string _mode = data.at("mode").get<std::string>();

            if (_mode.compare(common::CONFIG_CONTROL_MODE_ALL_GPIO) == 0) {
              const bool state = data.at("state").get<bool>();

              std::map<std::string, std::any> _data;

              _data["state"] = state;
              _data["mode"] = ModeControl::ALL;

              self->notify(
                  self->_service, CentralServices::RELAY,
                  new RecievePayload_2<RelayType,
                                       std::map<std::string, std::any>>(
                      ServiceType::EVENT_CHANGE_STATE, _data));
              /* ack again */
              self->sendMessage(ChannelMqtt::CHANNEL_IO_OUTPUT, data.dump());
            } else if (_mode.compare(
                           common::CONFIG_CONTROL_MODE_MULTIPLE_GPIO) == 0) {
              const std::vector<int> pos =
                  data.at("pos").get<std::vector<int>>();
              json::value_t type = data.at("state").type();
            } else if (_mode.compare(common::CONFIG_CONTROL_MODE_SINGLE_GPIO) ==
                       0) {
              const int pos = data.at("pos").get<int>();
              const bool state = data.at("state").get<bool>();

              std::map<std::string, std::any> _data;

              _data["state"] = state;
              _data["pos"] = pos;
              _data["mode"] = ModeControl::SINGLE;

              self->notify(
                  self->_service, CentralServices::RELAY,
                  new RecievePayload_2<RelayType,
                                       std::map<std::string, std::any>>(
                      ServiceType::EVENT_CHANGE_STATE, _data));
              /* ack again */
              self->sendMessage(ChannelMqtt::CHANNEL_IO_OUTPUT, data.dump());
            }
          }
#ifdef CONFIG_MODE_NODE

          /*
            {
              "state": true,
              "force": false,
              "_type": "buzzer",
            }
          */

          else if (_type.compare(common::CONFIG_CONTROL_TYPE_BUZZER) == 0) {
            const bool state = data.at("state").get<bool>();

            if (state) {
              self->notify(self->_service, CentralServices::BUZZER,
                           new RecievePayload_2<BuzzerType, nullptr_t>(
                               ServiceType::EVENT_BUZZER_START, nullptr));
            } else {
              self->notify(self->_service, CentralServices::BUZZER,
                           new RecievePayload_2<BuzzerType, nullptr_t>(
                               ServiceType::EVENT_BUZZER_STOP, nullptr));
            }
          }
#endif
        }
        /* handle message from topic config */
        else if (_gb_channel_topic[ChannelMqtt::CHANNEL_CONFIG].compare(
                     topic) == 0) {
          std::string _code = data.at("code").get<std::string>();

#ifdef CONFIG_MODE_NODE
          if (_code.compare(common::CONFIG_NOTIFY_SYNC_THRESHOLD) == 0) {
            cacheManager.thresholds_temp[0] =
                data["data"]["threshold"]["temperature"]["start"].get<int>();
            cacheManager.thresholds_humi[0] =
                data["data"]["threshold"]["humidity"]["start"].get<int>();
            cacheManager.thresholds_smoke[0] =
                data["data"]["threshold"]["smoke"]["start"].get<int>();

            /* create task to sync threshold */
            xTaskCreateWithCaps(&Mqtt::notifySyncThresholdTrigger,
                                "notifySyncThresholdTrigger", 4 * 1024, self, 6,
                                NULL, MALLOC_CAP_SPIRAM);
          }
#endif
        } else if (_gb_channel_topic[ChannelMqtt::CHANNEL_NOTIFY].compare(
                       topic) == 0) {
          std::string _code = data.at("code").get<std::string>();

          if (_code.compare(common::CONFIG_NOTIFY_REMOVE_DEVICE) == 0) {
            /* remove device */
            self->notify(self->_service, CentralServices::REFACTOR,
                         new RecievePayload_2<RefactorType, nullptr_t>(
                             RefactorType::REFACTOR_ALL, nullptr));
          }
        }
      } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
      }

      free(evt); // release memory
    }
  }
}

void Mqtt::start(void) {
  if (this->_isStarted) {
    ESP_LOGW(TAG, "Mqtt is started");
    return;
  }
  ESP_LOGI(TAG, "Mqtt start");
  xTaskCreateWithCaps(&Mqtt::init, "init::Mqtt", 4 * 1024, this, 6, NULL,
                      MALLOC_CAP_SPIRAM);
}

void Mqtt::stop(void) {
  if (!this->_isStarted) {
    ESP_LOGW(TAG, "Mqtt is stopped");
    return;
  }

  _eventGroupStop =
      (_eventGroupStop == nullptr) ? xEventGroupCreate() : _eventGroupStop;

  ESP_LOGI(TAG, "Mqtt stop");
  xTaskCreateWithCaps(&Mqtt::deinit, "deinit::Mqtt", 4 * 1024, this, 6, NULL,
                      MALLOC_CAP_SPIRAM);

  /* wait task done */
  xEventGroupWaitBits(_eventGroupStop, BIT_STOP, pdTRUE, pdFALSE,
                      pdMS_TO_TICKS(5000)); // just 5s
}

void Mqtt::init(void *arg) {
  Mqtt *self = static_cast<Mqtt *>(arg);

  if (cacheManager.m_username.length() == 0 ||
      cacheManager.m_password.length() == 0) {
    ESP_LOGE(TAG, "Username or password is empty.");
    vTaskDeleteWithCaps(NULL);
  }

  std::string url = std::string("mqtt://") + std::string(CONFIG_HOST_SERVER) +
                    std::string(":") + std::to_string(CONFIG_HOST_MQTT_PORT);
  std::string clientId = helperString::genClientId();

  esp_mqtt_client_config_t mqtt_cfg = {
      .broker{
          .address =
              {
                  .uri = url.c_str(),
              },
      },
      .credentials{.username = cacheManager.m_username.c_str(),
                   .client_id = clientId.c_str(),
                   .authentication =
                       {
                           .password = cacheManager.m_password.c_str(),
                       }},
      .session{
          .keepalive = 15, // 5s
      },
      .task{
          .priority = 5,
          .stack_size = 8 * 1024,
      }};

  ESP_LOGI(TAG, "MQTT ClientId: %s", mqtt_cfg.credentials.client_id);

  esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(client, MQTT_EVENT_ANY,
                                 self->mqtt_event_handler, self);
  esp_mqtt_client_start(client);
  _gb_client = client;

  if (_queueReciveMsg == nullptr) {
    _queueReciveMsg = xQueueCreate(10, sizeof(esp_mqtt_event_handle_t));
  }

  if (_taskRecieveMsg == nullptr) {
    xTaskCreate(&self->recieveMsg, "recieveMsg", 8 * 1024, self, 5,
                &_taskRecieveMsg);
  }

  self->_isStarted = true;
  vTaskDeleteWithCaps(NULL);
}

void Mqtt::deinit(void *arg) {
  Mqtt *self = static_cast<Mqtt *>(arg);
  /* handle deinit Mqtt */

  if (_gb_client != nullptr) {
    /* step: ungister event mqtt */
    esp_mqtt_client_unregister_event(_gb_client, MQTT_EVENT_ANY,
                                     self->mqtt_event_handler);

    /* step: unsub toppic */
    for (auto &topic : _gb_channel_topic) {
      if (topic.second.length() > 0) {
        esp_mqtt_client_unsubscribe(_gb_client, topic.second.c_str());
      }
    }

    /* step: stop recieve msg */
    if (_taskRecieveMsg != nullptr) {
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
  if (_eventGroupStop) {
    xEventGroupSetBits(_eventGroupStop, BIT_STOP);
  }

  self->_isStarted = false;
  vTaskDeleteWithCaps(NULL);
}

void Mqtt::onReceive(CentralServices s, void *data) {
  ESP_LOGI(TAG, "Mqtt onRecive from %d", s);
  switch (s) {
  case CentralServices::MESH: {
    RecievePayload_2<MqttEventType, nullptr_t> *payload =
        static_cast<RecievePayload_2<MqttEventType, nullptr_t> *>(data);
    if (payload->type == MqttEventType::EVENT_MQTT_START) {
      /* start mesh service */
      this->start();
    }
    delete payload;
    break;
  }
  case CentralServices::API_CALLER: {
    RecievePayload_2<MqttEventType, nullptr_t> *payload =
        static_cast<RecievePayload_2<MqttEventType, nullptr_t> *>(data);
    if (payload->type == MqttEventType::EVENT_MQTT_START) {
      /* start mesh service */
      this->start();
    }
    delete payload;
    break;
  }
  case CentralServices::SENSOR: {
    RecievePayload_2<SensorType, std::string> *payload =
        static_cast<RecievePayload_2<SensorType, std::string> *>(data);
    if (payload->type == SENSOR_SEND_SAMPLE_DATA) {
      this->sendMessage(ChannelMqtt::CHANNEL_SENSOR, payload->data);
    }
    delete payload;
    break;
  }

  default:
    break;
  }
}