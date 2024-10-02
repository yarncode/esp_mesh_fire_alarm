#pragma once

#include "esp_sntp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "Observer.h"
#include "ConstType.h"
#include "EventType.hpp"

class Sntp : public Observer
{
public:
  Sntp()
  {
    this->_service = CentralServices::SNTP;
    this->_running = false;
    this->_instance = this;
  };
  static void callbackService(timeval *tv);
  void restart(void);
  void setUpdateInterval(uint32_t ms, bool immediate = false);
  bool sntpState(void) { return this->_running; }
  const auto getTime(void);
  const std::string getTimeDesc(void);

  void onReceive(CentralServices s, void *data) override;
  void start(void);
  void stop(void);

private:
  static void init(void *arg);
  static void deinit(void *arg);
  static void restartModule(void *arg);

  bool _running;
  static Sntp *_instance;
};
