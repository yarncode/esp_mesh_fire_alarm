#ifndef CONST_TYPE_H
#define CONST_TYPE_H

#include <string>
#include <vector>
#include <iostream>
#include <any>
#include <map>
#include <vector>

#include "sdkconfig.h"

enum CentralServices
{
  NONE,
  ENCODER,
  STORAGE,
  MESH,
  LOG,
  MQTT,
  BLUETOOTH,
  REFACTOR,
  BUTTON,
  LED,
  API_CALLER,
  SENSOR,
  SNTP,
  LCD,
  RELAY,
  BUZZER,
  ROCKET_SCRIPT,
};

namespace common
{
  static const std::string CONFIG_PATH_FILE_WIFI_CONFIG = "/config_wifi.txt";
  static const std::string CONFIG_PATH_FILE_SERVER_CONFIG = "/config_server.txt";
  static const std::string CONFIG_PATH_FILE_THRESHOLD_CONFIG = "/config_trigger.txt";
#ifdef CONFIG_MODE_GATEWAY
  static const std::string CONFIG_NODE_TYPE_DEVICE = "GATEWAY";
#elif CONFIG_MODE_NODE
  static const std::string CONFIG_NODE_TYPE_DEVICE = "NODE";
#endif
  static const int CONFIG_PORT = 5000;
  static const std::string CONFIG_SERVER_IP = "127.0.0.1";
  static const std::string CONFIG_SERVER_HOST = "localhost";
  static const std::string CONFIG_SERVER_PROTOCOL = "http";

  /* -1: no limit, -2: disabled */
  static const std::vector<int> CONFIG_THRESHOLD_TEMP = std::vector<int>{50, -1};
  static const std::vector<int> CONFIG_THRESHOLD_HUMI = std::vector<int>{-2, -2};
  static const std::vector<int> CONFIG_THRESHOLD_SMOKE = std::vector<int>{500, -1};

  static const std::string CONFIG_NOTIFY_SYNC_THRESHOLD = "sync_threshold";

  static const std::string CONFIG_CONTROL_TYPE_GPIO = "gpio";
  static const std::string CONFIG_CONTROL_TYPE_BUZZER = "buzzer";

  static const std::string CONFIG_CONTROL_MODE_ALL_GPIO = "all";
  static const std::string CONFIG_CONTROL_MODE_MULTIPLE_GPIO = "mutiple";
  static const std::string CONFIG_CONTROL_MODE_SINGLE_GPIO = "single";
}

#endif // CONST_TYPE_H
