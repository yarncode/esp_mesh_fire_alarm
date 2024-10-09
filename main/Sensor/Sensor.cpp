#include "Sensor.hpp"

#include "MQSensor.h"

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

MQUnifiedsensor MQ2("ESP32", 5, 12, GPIO_NUM_34, "MQ-2");

void Sensor::sampleValue(void *arg)
{
  Sensor *self = static_cast<Sensor *>(arg);
  uint8_t random;
  double temperature, humidity, smoke;
  json body = json::object();

  while (true)
  {
    /* update data sensor MQ2 */
    MQ2.update();

    /* get random */
    random = esp_random();
    /* get temperature from random */
    temperature = random % 100;
    /* get humidity from random */
    humidity = random % 150;
    /* get smoke from data */
    smoke = MQ2.readSensor();

    // body["temperature"] = temperature;
    // body["humidity"] = humidity;
    body["smoke"] = smoke;

    ESP_LOGI(TAG, "Sensor sampleValue: %s", body.dump(2).c_str());

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

  MQ2.setRegressionMethod(1); //_PPM =  a*ratio^b
  /* Configure the equation to to calculate LPG concentration */
  MQ2.setA(987.99);
  MQ2.setB(-2.162);
  MQ2.setRL(1);
  MQ2.init();

  /* Start calibrate */
  ESP_LOGI(TAG, "Calibrate R0");
  float calcR0 = 0;
  for (int i = 1; i <= 10; i++)
  {
    MQ2.update(); // Update data, the arduino will read the voltage from the analog pin
    calcR0 += MQ2.calibrate(9.83);
  }
  MQ2.setR0(calcR0 / 10);
  ESP_LOGI(TAG, "Calibrate done.");

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