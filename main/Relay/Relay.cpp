#include "Relay.hpp"
#include "Arduino.h"
#include "Cache.h"
#include "Helper.hpp"
#include "Mqtt.hpp"
#include "Mesh.hpp"

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

using namespace nlohmann;

using namespace ServicePayload;
using namespace ServiceType;

static const char *TAG = "Relay";

void Relay::start(void) {
  ESP_LOGI(TAG, "Relay start");
  xTaskCreateWithCaps(&Relay::init, "Relay::init", 4 * 1024, this, 5, NULL,
                      MALLOC_CAP_SPIRAM);
}

void Relay::stop(void) {
  ESP_LOGI(TAG, "Relay stop");
  xTaskCreateWithCaps(&Relay::deinit, "Relay::deinit", 4 * 1024, this, 5, NULL,
                      MALLOC_CAP_SPIRAM);
}

void Relay::setStateRelay(int pin, bool state, bool saveNvs, bool syncOnline,
                          bool syncOtherNode) {
  digitalWrite(common::CONFIG_GPIO_OUTPUT.at(pin).gpio, state ? HIGH : LOW);
  /* set cache state */
  if (cacheManager.output_state[pin] != state) {
    cacheManager.output_state[pin] = state;
    /* storage nvs */
    if (saveNvs) {
      nvs_set_bool(common::CONFIG_GPIO_OUTPUT.at(pin).name, state);
    }

    if (syncOnline) {
      ESP_LOGI(TAG, "service type: %d", (int)this->_service);
      Mqtt *mqtt =
          static_cast<Mqtt *>(this->getObserver(CentralServices::MQTT));
      json _body;

      _body["timestamp"] = std::chrono::system_clock::to_time_t(
          std::chrono::system_clock::now());
      _body["state"] = state;
      _body["pos"] = pin;
      _body["type"] = common::CONFIG_KEY_OUTPUT;
      _body["mode"] = common::CONFIG_CONTROL_MODE_SINGLE_GPIO;

      /* send to broker */
      mqtt->sendMessage(ChannelMqtt::CHANNEL_IO_OUTPUT, _body.dump());
    }

    if(syncOtherNode) {
      /* send to other node */

      Mesh *mesh = static_cast<Mesh *>(this->getObserver(CentralServices::MESH));

      json body;

      body["timestamp"] =
          std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      body["code"] = common::CONFIG_MESH_CODE_SET_RELAY;
      body["state"] = state;
      body["pos"] = pin;

      mesh->sendMessageAllNode((uint8_t *)body.dump().c_str(), body.dump().length());
    }
  }
}

void Relay::init(void *arg) {
  Relay *self = static_cast<Relay *>(arg);

  /* setup gpio output */
  int index = 0;
  for (auto it = common::CONFIG_GPIO_OUTPUT.begin();
       it != common::CONFIG_GPIO_OUTPUT.end(); it++) {
    gpio_num_t pin = it->second.gpio;
    pinMode(pin, OUTPUT);
    if (digitalRead(pin) != cacheManager.output_state[index]) {
      digitalWrite(pin, cacheManager.output_state[index]);
    }
    index++;
  }

  vTaskDeleteWithCaps(NULL);
}

void Relay::deinit(void *arg) {
  Relay *self = static_cast<Relay *>(arg);

  vTaskDeleteWithCaps(NULL);
}

void Relay::onReceive(CentralServices s, void *data) {
  try {
    /* code */
    ESP_LOGI(TAG, "Relay onReceive: %d", s);

    switch (s) {
    case CentralServices::MQTT: {
      RecievePayload_2<RelayType, std::map<std::string, std::any>> *payload =
          static_cast<
              RecievePayload_2<RelayType, std::map<std::string, std::any>> *>(
              data);

      if (payload->type == EVENT_CHANGE_STATE) {
        Mqtt *mqtt =
            static_cast<Mqtt *>(this->getObserver(CentralServices::MQTT));
        ModeControl mode = std::any_cast<ModeControl>(payload->data.at("mode"));

        if (mode == ModeControl::SINGLE) {
          /* change state relay */
          int pos = std::any_cast<int>(payload->data.at("pos"));
          bool state = std::any_cast<bool>(payload->data.at("state"));

          this->setStateRelay(pos, state);
        } else if (mode == ModeControl::MULTIPLE) {
        } else if (mode == ModeControl::ALL) {
          bool state = std::any_cast<bool>(payload->data.at("state"));

          int index = 0;
          for (auto it = common::CONFIG_GPIO_OUTPUT.begin();
               it != common::CONFIG_GPIO_OUTPUT.end(); it++) {
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
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
}