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

class Buzzer : public Observer
{
public:
  Buzzer()
  {
    this->_service = CentralServices::BUZZER;
    this->ignoreWarning = false;
  };
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);

  void startWarning(bool syncWarn = false);
  void stopWarning(bool syncWarn = false);
  bool stateWarning(void);
  void setIgnoreWarning(bool ignore) { this->ignoreWarning = ignore; }
  bool getIgnoreWarning(void) { return this->ignoreWarning; }

private:
  static void _startWarning(void *arg);
  static void init(void *arg);
  static void deinit(void *arg);

  bool _stateWarning = false;
  bool ignoreWarning;
};
