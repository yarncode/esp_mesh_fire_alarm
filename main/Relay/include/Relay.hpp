#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "Observer.h"
#include "ConstType.h"
#include "EventType.hpp"

enum ModeControl {
  SINGLE,
  MULTIPLE,
  ALL,
};

class Relay : public Observer
{
public:
  Relay()
  {
    this->_service = CentralServices::RELAY;
  };
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);
  void setStateRelay(int pin, bool state);

private:
  static void init(void *arg);
  static void deinit(void *arg);
};
