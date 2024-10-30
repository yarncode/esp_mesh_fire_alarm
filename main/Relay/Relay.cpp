#include "Relay.hpp"

#include "Arduino.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace ServicePayload;
using namespace ServiceType;

static const char *TAG = "Relay";

static const std::map<int, gpio_num_t> _map_gpio = {
#ifdef CONFIG_MODE_GATEWAY
    {0, GPIO_NUM_25},
    {1, GPIO_NUM_26},
    {2, GPIO_NUM_27},
#elif CONFIG_MODE_NODE
    {0, GPIO_NUM_25},
#endif
};

void Relay::start(void)
{
  ESP_LOGI(TAG, "Relay start");
  xTaskCreate(&Relay::init, "Relay::init", 4 * 1024, this, 5, NULL);
}

void Relay::stop(void)
{
  ESP_LOGI(TAG, "Relay stop");
  xTaskCreate(&Relay::deinit, "Relay::deinit", 4 * 1024, this, 5, NULL);
}

void Relay::setStateRelay(gpio_num_t pin, bool state)
{
  digitalWrite(pin, state ? HIGH : LOW);
}

void Relay::init(void *arg)
{
  Relay *self = static_cast<Relay *>(arg);

  /* setup gpio */
  pinMode(GPIO_NUM_25, OUTPUT);

  // while (true)
  // {
  //   digitalWrite(GPIO_NUM_27, HIGH);
  //   vTaskDelay(500 / portTICK_PERIOD_MS);
  //   digitalWrite(GPIO_NUM_27, LOW);
  //   vTaskDelay(500 / portTICK_PERIOD_MS);
  // }

  vTaskDelete(NULL);
}

void Relay::deinit(void *arg)
{
  Relay *self = static_cast<Relay *>(arg);

  vTaskDelete(NULL);
}

void Relay::onReceive(CentralServices s, void *data)
{
  try
  {
    /* code */
    ESP_LOGI(TAG, "Relay onReceive: %d", s);

    switch (s)
    {
    case CentralServices::MQTT:
    {
      RecievePayload_2<RelayType, std::map<std::string, std::any>> *payload = static_cast<RecievePayload_2<RelayType, std::map<std::string, std::any>> *>(data);

      if (payload->type == EVENT_CHANGE_STATE)
      {
        ModeControl mode = std::any_cast<ModeControl>(payload->data.at("mode"));

        if (mode == ModeControl::SINGLE)
        {
          /* change state relay */
          int pos = std::any_cast<int>(payload->data.at("pos"));
          bool state = std::any_cast<bool>(payload->data.at("state"));
          this->setStateRelay(_map_gpio.at(pos), state);
        }
        else if (mode == ModeControl::MULTIPLE)
        {
        }
        else if (mode == ModeControl::ALL)
        {
          bool state = std::any_cast<bool>(payload->data.at("state"));

          for (auto it = _map_gpio.begin(); it != _map_gpio.end(); it++)
          {
            this->setStateRelay(it->second, state);
          }
        }
      }
      delete payload;
      break;
    }
    default:
      break;
    }
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << '\n';
  }
}