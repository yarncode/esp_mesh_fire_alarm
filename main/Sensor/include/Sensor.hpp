#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_random.h"

#include "Observer.h"
#include "ConstType.h"
#include "EventType.hpp"

class Sensor : public Observer
{
public:
  Sensor()
  {
    this->_service = CentralServices::SENSOR;
  };
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);

private:
  static void init(void *arg);
  static void sampleValue(void *arg);
  static void deinit(void *arg);
};
