#include "Sensor.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "nlohmann/json.hpp"

using namespace nlohmann;

using namespace ServiceType;
using namespace ServicePayload;

static const char *TAG = "Sensor";
static TaskHandle_t _taskSampleValue = NULL;

void Sensor::sampleValue(void *arg)
{
  Sensor *self = static_cast<Sensor *>(arg);
  uint8_t random;
  float temperature, humidity;
  uint16_t smoke;
  json body;

  while (true)
  {
    /* get random */
    random = esp_random();
    /* get temperature from random */
    temperature = random / 100.0f;
    /* get humidity from random */
    humidity = random / 100.0f;
    /* get smoke value from random 100pmm -> 1000pmm */
    smoke = esp_random() % 1000;

    body["temperature"] = temperature;
    body["humidity"] = humidity;
    body["smoke"] = smoke;

    self->notify(self->_service, CentralServices::MQTT, new RecievePayload_2<SensorType, std::string>(SENSOR_SEND_SAMPLE_DATA, body.dump()));

    /* get value every 5 seconds */
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }

  vTaskDelete(NULL);
}

void Sensor::start(void)
{
  ESP_LOGI(TAG, "Sensor start");
  xTaskCreate(&Sensor::init, "Sensor::init", 4 * 1024, this, 5, NULL);
}

void Sensor::stop(void)
{
  ESP_LOGI(TAG, "Sensor stop");
  xTaskCreate(&Sensor::deinit, "Sensor::deinit", 4 * 1024, this, 5, NULL);
}

void Sensor::init(void *arg)
{
  Sensor *self = static_cast<Sensor *>(arg);

  ESP_LOGI(TAG, "Sensor init");
  xTaskCreate(&Sensor::sampleValue, "Sensor::sampleValue", 4 * 1024, self, 5, &_taskSampleValue);

  vTaskDelete(NULL);
}

void Sensor::deinit(void *arg)
{
  Sensor *self = static_cast<Sensor *>(arg);

  ESP_LOGI(TAG, "Sensor deinit");

  if (_taskSampleValue != NULL)
  {
    vTaskDelete(_taskSampleValue);
    _taskSampleValue = NULL;
  }

  vTaskDelete(NULL);
}

void Sensor::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "Sensor onReceive: %d", s);
}