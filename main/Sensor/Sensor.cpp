#include "Sensor.hpp"
#include "Cache.h"
#include "Buzzer.hpp"

#include "MQSensor.h"
#include "sht31.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cmath>

#include "nlohmann/json.hpp"

using namespace nlohmann;

using namespace ServiceType;
using namespace ServicePayload;

static const char *TAG = "Sensor";
static TaskHandle_t _taskSampleValue = NULL;

/* sht31 */

/* mq2 */
MQUnifiedsensor MQ2("ESP32", 5, 12, GPIO_NUM_34, "MQ-2");

void Sensor::sampleValue(void *arg)
{
  Sensor *self = static_cast<Sensor *>(arg);
  Buzzer *buzzer = static_cast<Buzzer *>(self->getObserver(CentralServices::BUZZER));
  float temperature, humidity;
  double smoke;
  json body;
  bool flag_warning = false;
  time_t time_now;
  time_t time_last = 0;

  while (true)
  {
    /* reset state warning */
    flag_warning = false;

    /* update data sensor MQ2 */
    MQ2.update();

    /* get temperature and humidity from sht31 */
    sht31_readTempHum();

    humidity = sht31_readHumidity();
    temperature = sht31_readTemperature();

    time_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    /* get smoke from data */
    smoke = MQ2.readSensor();

    body["smoke"] = std::isnan(smoke) || std::isinf(smoke) ? 0.0 : std::ceil(smoke * 100.0) / 100.0;
    body["temperature"] = std::isnan(temperature) ? 0.0 : std::ceil(temperature * 100.0) / 100.0;
    body["humidity"] = std::isnan(humidity) ? 0.0 : std::ceil(humidity * 100.0) / 100.0;

    /* log temp, humi, smoke */
    // ESP_LOGI(TAG, "Sensor sampleValue: %f, %f, %f", temperature, humidity, smoke);
    ESP_LOGI(TAG, "Sensor sampleValue: %s", body.dump(2).c_str());

    self->notify(self->_service, CentralServices::MQTT, new RecievePayload_2<SensorType, std::string>(SENSOR_SEND_SAMPLE_DATA, body.dump()));

    /* check warning */
    if (temperature > cacheManager.thresholds_temp[0])
    {
      flag_warning = true;
    }

    if (smoke > cacheManager.thresholds_smoke[0])
    {
      flag_warning = true;
    }

    if (flag_warning)
    {
      /* debounce for 10 seconds */
      if (buzzer->stateWarning() == false && time_now - time_last > 10)
      {
        buzzer->startWarning();
        time_last = time_now;
      }
    }
    else
    {
      /* check state warning & stop */
      if (buzzer->stateWarning())
      {
        buzzer->stopWarning();
      }
    }

    /* get value every 5 seconds */
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }

  vTaskDeleteWithCaps(NULL);
}

void Sensor::start(void)
{
  ESP_LOGI(TAG, "Sensor start");
  xTaskCreateWithCaps(&Sensor::init, "Sensor::init", 4 * 1024, this, 5, NULL, MALLOC_CAP_SPIRAM);
}

void Sensor::stop(void)
{
  ESP_LOGI(TAG, "Sensor stop");
  xTaskCreateWithCaps(&Sensor::deinit, "Sensor::deinit", 4 * 1024, this, 5, NULL, MALLOC_CAP_SPIRAM);
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

  /* setup gpio input */
  int index = 0;
  for (auto it = common::CONFIG_GPIO_INPUT.begin(); it != common::CONFIG_GPIO_INPUT.end(); it++)
  {
    gpio_num_t pin = it->second.gpio;
    pinMode(pin, INPUT);
    cacheManager.input_state[index] = digitalRead(pin);
    index++;
  }

  /* Initialize sht31 */
  sht31_init();

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
  xTaskCreateWithCaps(&Sensor::sampleValue, "Sensor::sampleValue", 4 * 1024, self, 5, &_taskSampleValue, MALLOC_CAP_SPIRAM);

  vTaskDeleteWithCaps(NULL);
}

void Sensor::deinit(void *arg)
{
  Sensor *self = static_cast<Sensor *>(arg);

  ESP_LOGI(TAG, "Sensor deinit");

  if (_taskSampleValue != NULL)
  {
    vTaskDeleteWithCaps(_taskSampleValue);
    _taskSampleValue = NULL;
  }

  vTaskDeleteWithCaps(NULL);
}

void Sensor::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "Sensor onReceive: %d", s);
}