#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/timers.h>

#include "Observer.h"
#include "ConstType.h"
#include "EventType.hpp"

class Refactor : public Observer
{
public:
  Refactor()
  {
    this->_service = CentralServices::REFACTOR;
  };
  void onReceive(CentralServices s, void *data) override;

private:
  static void runAll(void *arg);
};
