#include "Button.hpp"

#include "Led.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace ServicePayload;
using namespace ServiceType;

static const char *TAG = "Button";
static std::map<uint8_t, FNMode> _select_mode = {
    {0, FNMode::FN_NONE},
    {1, FNMode::FN_CONFIG},
};
static std::map<uint8_t, led_custume_mode_t> _led_mode = {
    {1, LED_LOOP_500MS},
    {2, LED_LOOP_200MS},
};

void Button::start(void)
{
  ESP_LOGI(TAG, "Button start");
  xTaskCreate(&Button::init, "Button::init", 4 * 1024, this, 5, NULL);
}

void Button::stop(void)
{
  ESP_LOGI(TAG, "Button stop");
  xTaskCreate(&Button::deinit, "Button::deinit", 4 * 1024, this, 5, NULL);
}

void Button::handleFNMode(uint8_t mode)
{
  if (_select_mode.find(mode) != _select_mode.end())
  {
    FNMode fn_mode = _select_mode[mode];
    switch (fn_mode)
    {
    case FNMode::FN_NONE:
    {
      ESP_LOGI(TAG, "FN_NONE");
      break;
    }
    case FNMode::FN_CONFIG:
    {
      ESP_LOGI(TAG, "FN_CONFIG");
      this->notify(this->_service, CentralServices::BLUETOOTH, new RecievePayload_2<ButtonType, nullptr_t>(ServiceType::EVENT_BUTTON_CONFIG, nullptr));
      break;
    }
    default:
      break;
    }
  }
}

void Button::init(void *arg)
{
  Button *self = static_cast<Button *>(arg);

  button_event_t ev;
  QueueHandle_t button_events = button_init(PIN_BIT(GPIO_NUM_4));
  uint8_t mode = 0;

  while (true)
  {
    if (xQueueReceive(button_events, &ev, 1000 / portTICK_PERIOD_MS))
    {
      if (ev.pin == GPIO_NUM_4)
      {
        if (ev.event == BUTTON_DOWN)
        {
          // ESP_LOGI(TAG, "Button pressed");
        }
        else if (ev.event == BUTTON_UP)
        {
          // ESP_LOGI(TAG, "Button released");
          /* handle select mode here */
          self->handleFNMode(mode);
          /* reset mode */
          if (mode != 0)
          {
            /* turn off led */
            self->notify(self->_service, CentralServices::LED, new RecievePayload_2<LedType, led_custume_mode_t>(EVENT_LED_UPDATE_MODE, LED_MODE_NONE));
            mode = 0;
          }
        }
        else if (ev.event == BUTTON_HELD)
        {
          // ESP_LOGI(TAG, "Button held");
          mode++;
          if (_led_mode.contains(mode))
          {
            self->notify(self->_service, CentralServices::LED, new RecievePayload_2<LedType, led_custume_mode_t>(EVENT_LED_UPDATE_MODE, _led_mode.at(mode)));
          }
          else
          {
            /* turn off led because outside of mode select */
            self->notify(self->_service, CentralServices::LED, new RecievePayload_2<LedType, led_custume_mode_t>(EVENT_LED_UPDATE_MODE, LED_MODE_NONE));
          }
        }
      }
    }
  }

  vTaskDelete(NULL);
}

void Button::deinit(void *arg)
{
  Button *self = static_cast<Button *>(arg);

  vTaskDelete(NULL);
}

void Button::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "Button onReceive: %d", s);
}