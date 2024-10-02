#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "rom/rtc.h"
#include "esp_ota_ops.h"

#include "esp_mac.h"
#include "esp_wifi.h"
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_chip_info.h>
#include <esp_partition.h>
#include "esp_system.h"
#include "esp_flash.h"
#include "esp_image_format.h"
#include <esp_rom_spiflash.h>

static const char *MDF_SPACE_NAME = "ESP-MDF";
static const char *DEVICE_STORE_RESTART_COUNT_KEY = "restart_count";
static const int DEVICE_RESTART_TIMEOUT_MS = 10000;
static const int DEVICE_RESTART_COUNT_FALLBACK = 5;

bool nvs_set_number(std::string key, int number);
int nvs_get_number(std::string key);
bool nvs_set_bool(std::string key, bool value);
bool nvs_get_bool(std::string key);

esp_err_t
mdf_info_init(void);
esp_err_t mdf_info_save(const char *key, const void *value, size_t length);
esp_err_t mdf_info_load(const char *key, void *value, size_t length);
esp_err_t mdf_info_erase(const char *key);
esp_err_t mupgrade_version_fallback(void);

double calculateDownloadPercentage(unsigned long long bytesDownloaded, unsigned long long totalBytes);
std::string macToString(const uint8_t mac[6]);
std::string cronReplaceDayOfWeek(std::string str);
std::string ip4ToString(const esp_ip4_addr_t &ip4);

namespace helperString {
  std::vector<std::string> split(const std::string& str, char delimiter);
  std::vector<std::string> split(const std::string& str, const std::string& delimiter);
  std::string genClientId(void);
}

namespace chipInfo {

  typedef enum {
    SKETCH_SIZE_TOTAL = 0,
    SKETCH_SIZE_FREE = 1
  } sketchSize_t;

  std::string getMacDevice(void);
  std::string getMacBleDevice(void);
  uint8_t getChipCores(void);
  std::string getChipModel(void);
  uint32_t getSketchSize(sketchSize_t response);
  uint32_t getFreeSketchSpace(void);
  uint32_t ESP_getFlashChipId(void);
  uint32_t getFlashChipSize(void);
  uint16_t getChipRevision(void);

}
