#include "Buzzer.hpp"

#include "Arduino.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace ServicePayload;
using namespace ServiceType;

static const char *TAG = "Buzzer";

static TaskHandle_t _taskStartWarning = nullptr;

bool Buzzer::stateWarning(void)
{
  return _stateWarning;
}

void Buzzer::_startWarning(void *arg)
{
  Buzzer *self = static_cast<Buzzer *>(arg);

  self->_stateWarning = true;

  while (true)
  {
    digitalWrite(GPIO_NUM_27, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(GPIO_NUM_27, LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  vTaskDeleteWithCaps(NULL);
}

void Buzzer::startWarning(void)
{
  ESP_LOGI(TAG, "Buzzer startWarning");

  if (_taskStartWarning != nullptr)
  {
    return;
  }

  xTaskCreateWithCaps(&Buzzer::_startWarning, "Buzzer::_startWarning", 2 * 1024, this, 5, &_taskStartWarning, MALLOC_CAP_SPIRAM);
}

void Buzzer::stopWarning(void)
{
  ESP_LOGI(TAG, "Buzzer stopWarning");

  if (_taskStartWarning != nullptr)
  {
    vTaskDeleteWithCaps(_taskStartWarning);
    _taskStartWarning = nullptr;
  }
  
  this->_stateWarning = false;
}

void Buzzer::start(void)
{
  ESP_LOGI(TAG, "Buzzer start");
  xTaskCreate(&Buzzer::init, "Buzzer::init", 4 * 1024, this, 5, NULL);
}

void Buzzer::stop(void)
{
  ESP_LOGI(TAG, "Buzzer stop");
  xTaskCreate(&Buzzer::deinit, "Buzzer::deinit", 4 * 1024, this, 5, NULL);
}

void Buzzer::init(void *arg)
{
  Buzzer *self = static_cast<Buzzer *>(arg);

  /* setup gpio */
  pinMode(GPIO_NUM_27, OUTPUT);

  vTaskDelete(NULL);
}

void Buzzer::deinit(void *arg)
{
  Buzzer *self = static_cast<Buzzer *>(arg);

  vTaskDelete(NULL);
}

void Buzzer::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "Buzzer onReceive: %d", s);

  switch (s)
  {
  case CentralServices::MQTT:
  {
    RecievePayload_2<BuzzerType, nullptr_t> *payload = static_cast<RecievePayload_2<BuzzerType, nullptr_t> *>(data);

    if (payload->type == EVENT_BUZZER_START)
    {
      /* start warning service */
      this->startWarning();
    }
    else if (payload->type == EVENT_BUZZER_STOP)
    {
      /* stop warning service */
      this->stopWarning();
    }
    delete payload;
    break;
  }
  default:
    break;
  }
}