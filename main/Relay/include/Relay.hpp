#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "ConstType.h"
#include "EventType.hpp"
#include "Observer.h"

enum ModeControl {
  SINGLE,
  MULTIPLE,
  ALL,
};

class Relay : public Observer {
public:
  Relay() { this->_service = CentralServices::RELAY; };
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);
  void setStateRelay(int pin, bool state, bool saveNvs = true,
                     bool syncOnline = false, bool syncOtherNode = false);

private:
  static void init(void *arg);
  static void deinit(void *arg);
};
