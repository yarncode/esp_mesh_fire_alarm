#include "Relay.hpp"
#include "Cache.h"
#include "Arduino.h"
#include "Helper.hpp"
#include "Mqtt.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace ServicePayload;
using namespace ServiceType;

static const char *TAG = "Relay";

void Relay::start(void)
{
  ESP_LOGI(TAG, "Relay start");
  xTaskCreateWithCaps(&Relay::init, "Relay::init", 4 * 1024, this, 5, NULL, MALLOC_CAP_SPIRAM);
}

void Relay::stop(void)
{
  ESP_LOGI(TAG, "Relay stop");
  xTaskCreateWithCaps(&Relay::deinit, "Relay::deinit", 4 * 1024, this, 5, NULL, MALLOC_CAP_SPIRAM);
}

void Relay::setStateRelay(int pin, bool state)
{
  digitalWrite(common::CONFIG_GPIO_OUTPUT.at(pin).gpio, state ? HIGH : LOW);
  /* set cache state */
  if (cacheManager.output_state[pin] != state)
  {
    cacheManager.output_state[pin] = state;
    /* storage nvs */
    nvs_set_bool(common::CONFIG_GPIO_OUTPUT.at(pin).name, state);
  }
}

void Relay::init(void *arg)
{
  Relay *self = static_cast<Relay *>(arg);

  /* setup gpio output */
  int index = 0;
  for (auto it = common::CONFIG_GPIO_OUTPUT.begin(); it != common::CONFIG_GPIO_OUTPUT.end(); it++)
  {
    gpio_num_t pin = it->second.gpio;
    pinMode(pin, OUTPUT);
    if (digitalRead(pin) != cacheManager.output_state[index])
    {
      digitalWrite(pin, cacheManager.output_state[index]);
    }
    index++;
  }

  vTaskDeleteWithCaps(NULL);
}

void Relay::deinit(void *arg)
{
  Relay *self = static_cast<Relay *>(arg);

  vTaskDeleteWithCaps(NULL);
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
        Mqtt *mqtt = static_cast<Mqtt *>(this->getObserver(CentralServices::MQTT));
        ModeControl mode = std::any_cast<ModeControl>(payload->data.at("mode"));

        if (mode == ModeControl::SINGLE)
        {
          /* change state relay */
          int pos = std::any_cast<int>(payload->data.at("pos"));
          bool state = std::any_cast<bool>(payload->data.at("state"));

          this->setStateRelay(pos, state);
        }
        else if (mode == ModeControl::MULTIPLE)
        {
        }
        else if (mode == ModeControl::ALL)
        {
          bool state = std::any_cast<bool>(payload->data.at("state"));

          int index = 0;
          for (auto it = common::CONFIG_GPIO_OUTPUT.begin(); it != common::CONFIG_GPIO_OUTPUT.end(); it++)
          {
            this->setStateRelay(index, state);
            index++;
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