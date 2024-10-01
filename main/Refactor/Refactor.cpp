#include "Refactor.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace ServicePayload;
using namespace ServiceType;

static const char *TAG = "Refactor";

void Refactor::runAll(void *arg)
{
  ESP_LOGI(TAG, "Refactor runAll");
  Refactor *refactor = static_cast<Refactor *>(arg);

  /* refactor to storage */
  refactor->getObserver(CentralServices::STORAGE)->onReceive(refactor->_service, nullptr);
  /* refactor mesh */
  refactor->getObserver(CentralServices::MESH)->onReceive(refactor->_service, nullptr);

  vTaskDelay(2000 / portTICK_PERIOD_MS);
  esp_restart();

  vTaskDelete(NULL);
}

void Refactor::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "Refactor onReceive: %d", s);
  RecievePayload_2<RefactorType, nullptr_t> *event = static_cast<RecievePayload_2<RefactorType, nullptr_t> *>(data);

  if (event->type == REFACTOR_ALL)
  {
    xTaskCreate(this->runAll, "runAll", 4 * 1024, this, 5, NULL);
  }

  delete event;
}