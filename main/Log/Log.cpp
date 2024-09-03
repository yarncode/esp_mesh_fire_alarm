#include "Log.hpp"
#include "Mesh.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>

static const char *TAG = "Log";

void Log::start(void)
{
  ESP_LOGI(TAG, "Log start.");
  xTaskCreate(&Log::init, "Log", 4 * 1024, this, 10, NULL);
}

void Log::init(void *arg)
{
  Log *self = static_cast<Log *>(arg);
  Mesh *mesh = static_cast<Mesh *>(self->getObserver(CentralServices::MESH));

  while (true)
  {
    self->heapSize = esp_get_free_heap_size();

    /* log for esp native */
    ESP_LOGI(TAG, "| [*** RAM Monitor ***]");
    ESP_LOGI(TAG, "| Free heap size: %" PRIu32 " (bytes)", self->heapSize);

    /* log for mesh info */
    ESP_LOGI(TAG, "| [*** Mesh Monitor ***]");
    ESP_LOGI(TAG, "| Layer: %d", mesh->layer);
    ESP_LOGI(TAG, "| Parent MAC: " MACSTR, MAC2STR(mesh->parentAddress.addr));
    ESP_LOGI(TAG, "| Parent IP: " IPSTR, IP2STR(&mesh->parentAddress.mip.ip4));
    ESP_LOGI(TAG, "| Parent Port: %" PRId16, mesh->parentAddress.mip.port);
    ESP_LOGI(TAG, "| Node MAC: " MACSTR, MAC2STR(mesh->meshAddress.addr));
    ESP_LOGI(TAG, "| Node IP: " IPSTR, IP2STR(&mesh->currentIp));
    ESP_LOGI(TAG, "| Node Port: %" PRId16, mesh->meshAddress.mip.port);

    vTaskDelay(10 * 1000 / portTICK_PERIOD_MS); // log every 10 seconds
  }

  vTaskDelete(NULL);
}

void Log::onReceive(CentralServices s, void *data)
{
  std::cout << "Log onReceive" << std::endl;
  switch (s)
  {
  case CentralServices::STORAGE:
  {
    ServicePayload::RecievePayload<ServiceType::StorageEventType> *payload = static_cast<ServicePayload::RecievePayload<ServiceType::StorageEventType> *>(data);
    if (payload->type == ServiceType::StorageEventType::EVENT_STORAGE_STARTED)
    {
      /* start log service */
      this->start();
    }
    delete payload;
    break;
  }

  default:
    break;
  }
}