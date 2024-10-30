#include "RocketScript.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>

static const char *TAG = "RocketScript";

void RocketScript::start(void)
{
  ESP_LOGI(TAG, "RocketScript start");
  xTaskCreate(&RocketScript::init, "RocketScript::init", 4 * 1024, this, 5, NULL);
}

void RocketScript::stop(void)
{
  ESP_LOGI(TAG, "RocketScript stop");
  xTaskCreate(&RocketScript::deinit, "RocketScript::deinit", 4 * 1024, this, 5, NULL);
}

void RocketScript::init(void *arg)
{
  RocketScript *self = static_cast<RocketScript *>(arg);

  vTaskDelete(NULL);
}

void RocketScript::deinit(void *arg)
{
  RocketScript *self = static_cast<RocketScript *>(arg);

  vTaskDelete(NULL);
}

void RocketScript::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "RocketScript onReceive: %d", s);
}