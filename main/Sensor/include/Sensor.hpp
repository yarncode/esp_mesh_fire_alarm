#pragma once

#include "Arduino.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "driver/gpio.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "ConstType.h"
#include "EventType.hpp"
#include "Observer.h"

class Sensor : public Observer {
public:
  Sensor() { this->_service = CentralServices::SENSOR; };
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);

private:
  static void init(void *arg);
  static void sampleValue(void *arg);
  static void deinit(void *arg);
  static void gpioInterrupt(void *arg);
};
