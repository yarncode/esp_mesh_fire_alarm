#include "SNTP.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <map>
#include <chrono>

using namespace ServicePayload;
using namespace ServiceType;

using namespace std;
using namespace std::chrono;

static const char *TAG = "SNTP";

Sntp *Sntp::_instance = nullptr;

void Sntp::callbackService(timeval *tv)
{
  auto timeNow{system_clock::to_time_t(system_clock::now())};
  std::string timeDesc(std::asctime(std::localtime(&timeNow)));
  ESP_LOGI(TAG, "[NTP UPDATE][%s]", timeDesc.c_str());
}

void Sntp::restartModule(void *arg)
{
  Sntp *self = static_cast<Sntp *>(arg);
  ESP_LOGI(TAG, "Restarting SNTP");
  sntp_restart();
  vTaskDelete(NULL);
}

void Sntp::start(void)
{
  ESP_LOGI(TAG, "Sntp start");
  if (!this->_running)
  {
    xTaskCreate(&Sntp::init, "Sntp::init", 4 * 1024, this, 5, NULL);
  }
}

void Sntp::restart(void)
{
  if (this->_running)
  {
    xTaskCreate(this->restartModule, "Sntp::restart", 4 * 1024, this, 5, NULL);
  }
}

void Sntp::setUpdateInterval(uint32_t ms, const bool immediate)
{
  if (this->_running)
  {
    sntp_set_sync_interval(ms);
    if (immediate)
    {
      sntp_restart();
    }
  }
}

const auto Sntp::getTime(void)
{
  return std::chrono::system_clock::now();
}

const std::string Sntp::getTimeDesc(void)
{
  const std::time_t timeNow{std::chrono::system_clock::to_time_t(this->getTime())};
  return std::string(std::asctime(std::localtime(&timeNow)));
}

void Sntp::stop(void)
{
  ESP_LOGI(TAG, "Sntp stop");
  xTaskCreate(&Sntp::deinit, "Sntp::deinit", 4 * 1024, this, 5, NULL);
}

void Sntp::init(void *arg)
{
  Sntp *self = static_cast<Sntp *>(arg);

  ESP_LOGI(TAG, "Starting SNTP");

  setenv("TZ", "WIB-7", 1);
  tzset();

  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "time.google.com");
  sntp_setservername(1, "pool.ntp.com");

  sntp_set_time_sync_notification_cb(self->callbackService);
  sntp_set_sync_interval(60 * 60 * 1000); // Update time every hour

  sntp_init();

  ESP_LOGI(TAG, "SNTP Initialised");

  self->_running = true;

  vTaskDelete(NULL);
}

void Sntp::deinit(void *arg)
{
  Sntp *self = static_cast<Sntp *>(arg);
  vTaskDelete(NULL);
}

void Sntp::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "Sntp onReceive: %d", s);
  switch (s)
  {
  case CentralServices::MESH:
  {
    RecievePayload_2<SNTPType, nullptr_t> *payload = static_cast<RecievePayload_2<SNTPType, nullptr_t> *>(data);
    if (payload->type == SNTP_START)
    {
      /* start mesh service */
      this->start();
    }
    else if (payload->type == SNTP_RESTART)
    {
      this->restart();
    }
    delete payload;
    break;
  }
  default:
    break;
  }
}