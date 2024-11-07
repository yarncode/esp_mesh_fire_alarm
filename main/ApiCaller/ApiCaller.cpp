#include "ApiCaller.hpp"
#include "Cache.h"
#include "Helper.hpp"
#include "Storage.hpp"
#include "Mqtt.hpp"

#include "esp_http_client.h"
#include "nlohmann/json.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace nlohmann;
using namespace ServicePayload;
using namespace ServiceType;

static const char *TAG = "ApiCaller";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  static char *output_buffer; // Buffer to store response of http request from event handler
  static int output_len;      // Stores number of bytes read
  switch (evt->event_id)
  {
  case HTTP_EVENT_REDIRECT:
    ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
    break;
  case HTTP_EVENT_ERROR:
    ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    /*
     *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
     *  However, event handler can also be used in case chunked encoding is used.
     */
    if (!esp_http_client_is_chunked_response(evt->client))
    {
      // If user_data buffer is configured, copy the response into the buffer
      if (evt->user_data)
      {
        memcpy(evt->user_data + output_len, evt->data, evt->data_len);
      }
      else
      {
        if (output_buffer == NULL)
        {
          output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
          output_len = 0;
          if (output_buffer == NULL)
          {
            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
            return ESP_FAIL;
          }
        }
        memcpy(output_buffer + output_len, evt->data, evt->data_len);
      }
      output_len += evt->data_len;
    }

    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
    if (output_buffer != NULL)
    {
      // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
      // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
      free(output_buffer);
      output_buffer = NULL;
    }
    output_len = 0;
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  }
  return ESP_OK;
}

bool ApiCaller::isCallingCreateDevice(void)
{
  return cacheManager.activeToken.length() > 0 ? true : false;
}

bool ApiCaller::getCacheApiCallRecently(void)
{
  return this->_cache_api_call_recently;
}

void ApiCaller::setCacheApiCallRecently(bool value)
{
  this->_cache_api_call_recently = value;
}

void ApiCaller::addDevice(void *arg)
{
  ApiCaller *self = static_cast<ApiCaller *>(arg);

  if (cacheManager.activeToken.length() == 0)
  {
    ESP_LOGE(TAG, "Token not found");
    vTaskDelete(NULL);
    return;
  }


  char payload_response[512];
  memset(payload_response, 0, sizeof(payload_response));

  std::string path = std::string("/") + std::string(CONFIG_ENTRY_PATH) + std::string("/") + std::string(CONFIG_API_VERSION) + std::string("/device/new");

  esp_http_client_config_t config = {
      .host = CONFIG_HOST_SERVER,
      .port = CONFIG_HOST_API_PORT,
      .path = path.c_str(),
      .method = HTTP_METHOD_POST,
      .event_handler = _http_event_handler,
      .user_data = payload_response,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);

  /* set content type */
  esp_http_client_set_header(client, "Content-Type", "application/json");
  /* set token of user to header */
  esp_http_client_set_header(client, "_token", cacheManager.activeToken.c_str());
  /* add info device to body */
  json body;
  body["type_node"] = common::CONFIG_NODE_TYPE_DEVICE;
  body["mac"] = chipInfo::getMacDevice();

  std::string payload = body.dump();
  esp_http_client_set_post_field(client, payload.c_str(), payload.length());

  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK)
  {
    ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld",
             esp_http_client_get_status_code(client),
             esp_http_client_get_content_length(client));
    try
    {
      json j = json::parse(payload_response);
      ESP_LOGI(TAG, "response payload: %s", j.dump().c_str());

      std::string code = j["code"].get<std::string>();

      if (code == "107012" || code == "107010")
      {
        cacheManager.m_username = j["auth"]["username"];
        cacheManager.m_password = j["auth"]["password"];
        cacheManager.mainToken = j["auth"]["token"];

        /* save config */
        Storage storage;
        if (storage.saveServerConfig())
        {
          ESP_LOGI(TAG, "Save server config success");
        }

        self->setCacheApiCallRecently(true);
      }

      /* Start MQTT connection with new config */
      self->notify(self->_service, CentralServices::MQTT, new RecievePayload_2<MqttEventType, nullptr_t>(MqttEventType::EVENT_MQTT_START, nullptr));
    }
    catch (const std::exception &e)
    {
      std::cerr << e.what() << '\n';
    }
  }

  ESP_LOGI(TAG, "ApiCaller addDevice done.");

  /* reset token active */
  cacheManager.activeToken = "";
  esp_http_client_cleanup(client);

  vTaskDelete(NULL);
}

void ApiCaller::start(void)
{
  ESP_LOGI(TAG, "ApiCaller start");
  xTaskCreate(&ApiCaller::init, "ApiCaller::init", 4 * 1024, this, 5, NULL);
}

void ApiCaller::stop(void)
{
  ESP_LOGI(TAG, "ApiCaller stop");
  xTaskCreate(&ApiCaller::deinit, "ApiCaller::deinit", 4 * 1024, this, 5, NULL);
}

void ApiCaller::init(void *arg)
{
  ApiCaller *self = static_cast<ApiCaller *>(arg);

  vTaskDelete(NULL);
}

void ApiCaller::deinit(void *arg)
{
  ApiCaller *self = static_cast<ApiCaller *>(arg);

  vTaskDelete(NULL);
}

void ApiCaller::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "ApiCaller onReceive: %d", s);
  RecievePayload_2<ApiCallerType, nullptr_t> *payload = static_cast<RecievePayload_2<ApiCallerType, nullptr_t> *>(data);

  if (payload->type == EVENT_API_CALLER_ADD_DEVICE)
  {
    /* add device */
    ESP_LOGI(TAG, "ApiCaller add device...");
    xTaskCreate(&ApiCaller::addDevice, "addDevice", 8 * 1024, this, 5, NULL);
  }

  delete payload;
}