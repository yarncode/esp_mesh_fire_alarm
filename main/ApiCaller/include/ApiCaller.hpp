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

class ApiCaller : public Observer
{
public:
  ApiCaller()
  {
    this->_service = CentralServices::API_CALLER;
    this->_cache_api_call_recently = false;
  };
  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);
  bool isCallingCreateDevice(void);
  bool getCacheApiCallRecently(void);
  void setCacheApiCallRecently(bool value);

private:
  static void init(void *arg);
  static void deinit(void *arg);
  static void addDevice(void *arg);

  bool _cache_api_call_recently;
};
