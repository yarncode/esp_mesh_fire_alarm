#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "Observer.h"
#include "ConstType.h"
#include "EventType.hpp"

// #include <soc/gpio_num.h>
#include "driver/gpio.h"
#include "string.h"

typedef enum
{
  LED_MODE_NONE,
  LED_LOOP_50MS,
  LED_LOOP_200MS,
  LED_LOOP_500MS,
  LED_LOOP_DUP_SIGNAL,
  LED_LOOP_2DUP_SIGNAL,
} led_custume_mode_t;

class Led : public Observer
{
public:
  Led()
  {
    this->_service = CentralServices::NONE;
  };
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);
  void dispatchLedMode(led_custume_mode_t mode);
  void stopLoopLedStatus(void);
  void turnOffLed(gpio_num_t);

private:
  static void init(void *arg);
  static void deinit(void *arg);
  static void runLoopLedStatus(void *arg);
  led_custume_mode_t _select_led_mode;

};
