#include "Template.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>

static const char *TAG = "Template";

void Template::start(void)
{
  ESP_LOGI(TAG, "Template start");
  xTaskCreate(&Template::init, "Template::init", 4 * 1024, this, 5, NULL);
}

void Template::stop(void)
{
  ESP_LOGI(TAG, "Template stop");
  xTaskCreate(&Template::deinit, "Template::deinit", 4 * 1024, this, 5, NULL);
}

void Template::init(void *arg)
{
  Template *self = static_cast<Template *>(arg);

  vTaskDelete(NULL);
}

void Template::deinit(void *arg)
{
  Template *self = static_cast<Template *>(arg);

  vTaskDelete(NULL);
}

void Template::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "Template onReceive: %d", s);
}