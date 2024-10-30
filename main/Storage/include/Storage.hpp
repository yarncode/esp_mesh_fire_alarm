#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "Observer.h"
#include "ConstType.h"
#include "EventType.hpp"

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"

#include <nlohmann/json.hpp>

using namespace nlohmann;

class Storage : public Observer
{
public:
  Storage()
  {
    this->_service = CentralServices::STORAGE;
  };
  void onReceive(CentralServices s, void *data) override;
  void start(void);

  bool writeFile(std::string path, std::string data);
  std::string readFile(std::string path);
  void stop(void);
  bool nvsSetBool(std::string key, bool value);
  bool nvsGetBool(std::string key);
  static std::string serverConfig(void);

  bool nvsSetString(std::string key, std::string value);
  std::string nvsGetString(std::string key);
  std::string thresholdTriggerConfig(void);

  bool saveWifiConfig(void);
  bool saveServerConfig(void);
  bool saveThresholdTriggerConfig(void);

  bool loadServerConfig(void);
  bool loadWifiConfig(void);
  bool loadThresholdTriggerConfig(void);

  json generateDefaultWifiConfig(void);
  json generateDefaultServerConfig(void);
  json generateDefaultThresholdsConfig(void);
  
  static void handleFactory(void *arg);

private:
  static void init(void *arg);
  std::string _base_path = "/spiffs";
  bool _isStarted = false;
};
