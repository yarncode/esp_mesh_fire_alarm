#include "Led.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace ServicePayload;
using namespace ServiceType;

static const uint8_t LED_LOOP_DUP_SIGNAL_TIMES = 2;
static const char *TAG = "Led";
static const std::vector<gpio_num_t> _leds = {GPIO_NUM_2};
static const std::map<led_custume_mode_t, uint32_t> _led_mode = {
    {LED_LOOP_50MS, 50},
    {LED_LOOP_200MS, 200},
    {LED_LOOP_500MS, 500},
    {LED_LOOP_DUP_SIGNAL, 100},
};
static TaskHandle_t _loop_led_task = NULL;

void Led::runLoopLedStatus(void *arg)
{
  Led *self = static_cast<Led *>(arg);
  uint8_t loop_counter = LED_LOOP_DUP_SIGNAL_TIMES;
  uint32_t sleep_time = _led_mode.at(self->_select_led_mode);
  ESP_LOGI(TAG, "sleep time: %" PRIu32, sleep_time);
  while (true)
  {
    if (self->_select_led_mode == LED_LOOP_50MS ||
        self->_select_led_mode == LED_LOOP_200MS ||
        self->_select_led_mode == LED_LOOP_500MS)
    {
      gpio_set_level(GPIO_NUM_2, 1);
      vTaskDelay(sleep_time / portTICK_PERIOD_MS);
      gpio_set_level(GPIO_NUM_2, 0);
      vTaskDelay(sleep_time / portTICK_PERIOD_MS);
    }
    else if (self->_select_led_mode == LED_LOOP_DUP_SIGNAL)
    {
      loop_counter = LED_LOOP_DUP_SIGNAL_TIMES;
      while (loop_counter--)
      {
        gpio_set_level(GPIO_NUM_2, 1);
        vTaskDelay(sleep_time / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_2, 0);
        vTaskDelay(sleep_time / portTICK_PERIOD_MS);
      }
      vTaskDelay((sleep_time * LED_LOOP_DUP_SIGNAL_TIMES * 2) / portTICK_PERIOD_MS);
    }
  }
  vTaskDelete(NULL);
}

void Led::stopLoopLedStatus(void)
{
  if (_loop_led_task != NULL)
  {
    vTaskDelete(_loop_led_task);
    _loop_led_task = NULL;
  }
}

void Led::turnOffLed(gpio_num_t led) { gpio_set_level(led, 0); }

void Led::dispatchLedMode(led_custume_mode_t mode)
{
  /* stop led status presnt if running */
  this->stopLoopLedStatus();
  this->_select_led_mode = mode;
  if (mode == LED_MODE_NONE)
  {
    ESP_LOGI(TAG, "Led mode: NONE");
    /* turn off led */
    this->turnOffLed(GPIO_NUM_2);
    return;
  }
  if (_led_mode.contains(mode))
  {
    ESP_LOGI(TAG, "Led mode: %d", mode);
    xTaskCreate(&this->runLoopLedStatus, "runLoopLedStatus", 3 * 1024, this, 5, &_loop_led_task);
  }
}

void Led::start(void)
{
  ESP_LOGI(TAG, "Led start");
  xTaskCreate(&Led::init, "Led::init", 4 * 1024, this, 5, NULL);
}

void Led::stop(void)
{
  ESP_LOGI(TAG, "Led stop");
  xTaskCreate(&Led::deinit, "Led::deinit", 4 * 1024, this, 5, NULL);
}

void Led::init(void *arg)
{
  Led *self = static_cast<Led *>(arg);
  gpio_config_t io_conf;

  memset(&io_conf, 0, sizeof(gpio_config_t));

  // set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  // disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  // disable pull-down mode
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  // disable pull-up mode
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

  /* mapping pin to pin mask */
  for (const gpio_num_t &led : _leds)
  {
    io_conf.pin_bit_mask |= (1ULL << led);
  }

  /* start config */
  gpio_config(&io_conf);

  vTaskDelete(NULL);
}

void Led::deinit(void *arg)
{
  Led *self = static_cast<Led *>(arg);

  for (const gpio_num_t &led : _leds)
  {
    gpio_set_level(led, 0);
  }

  vTaskDelete(NULL);
}

void Led::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "Led onReceive: %d", s);
  RecievePayload_2<LedType, led_custume_mode_t> *payload = static_cast<RecievePayload_2<LedType, led_custume_mode_t> *>(data);

  if (payload->type == EVENT_LED_UPDATE_MODE)
  {
    this->dispatchLedMode(payload->data);
  }

  delete payload;
}