#include "Storage.hpp"
#include "Cache.h"

#include <nvs_flash.h>
#include "esp_spiffs.h"
#include <esp_log.h>
#include "esp_mac.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/timers.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>

using namespace ServicePayload;
using namespace ServiceType;

static const char *TAG = "Storage";

bool Storage::nvsSetString(std::string key, std::string value)
{
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  }
  else
  {
    err = nvs_set_str(nvs_handle, key.c_str(), value.c_str());
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Error (%s) writing!", esp_err_to_name(err));
    }
    nvs_close(nvs_handle);
    return true;
  }
  return false;
}

std::string Storage::nvsGetString(std::string key)
{
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  }
  else
  {
    size_t required_size;
    err = nvs_get_str(nvs_handle, key.c_str(), NULL, &required_size);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
    }
    char *value = (char *)malloc(required_size);
    err = nvs_get_str(nvs_handle, key.c_str(), value, &required_size);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
    }
    nvs_close(nvs_handle);
    return std::string(value);
  }
  return std::string("");
}

json Storage::generateDefaultWifiConfig(void)
{
  json j;
  j["ssid"] = "";
  j["password"] = "";
  j["bssid"] = std::vector<uint8_t>{0, 0, 0, 0, 0, 0};
  j["bssid_set"] = false;
  j["history_station_connected"] = false;

  return j;
}

json Storage::generateDefaultServerConfig(void)
{
  json j;
  j["serverHost"] = "";
  j["serverPort"] = (int)0;

  j["m_username"] = "";
  j["m_password"] = "";

  j["scheme"] = "";
  j["serverIp"] = "";
  j["protocol"] = "";
  j["mainToken"] = "";

  return j;
}

std::string Storage::readFile(std::string path)
{
  std::string filePath = this->_base_path + path;
  FILE *file = fopen(filePath.c_str(), "r");
  if (file == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file for reading");
    /* create file if file not found */
    file = fopen(filePath.c_str(), "w");
    if (file == NULL)
    {
      ESP_LOGE(TAG, "Failed to create file for writing");
      return std::string("");
    }
  }

  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (length == 0)
  {
    fclose(file);
    return std::string("");
  }

  char *buffer = (char *)malloc(length);

  if (buffer)
  {
    fread(buffer, 1, length, file);
  }
  fclose(file);

  /* convert buffer to std::string */
  std::string data(buffer, length);

  free(buffer);

  return data;
}

bool Storage::writeFile(std::string path, std::string data)
{
  std::string filePath = this->_base_path + path;
  FILE *file = fopen(filePath.c_str(), "w");
  if (file == NULL)
  {
    ESP_LOGE(TAG, "Failed to open file for writing");
    return false;
  }

  fwrite(data.c_str(), 1, data.length(), file);
  fclose(file);

  return true;
}

bool Storage::loadServerConfig(void)
{
  std::string data = this->readFile(common::CONFIG_PATH_FILE_SERVER_CONFIG);

  // ESP_LOGI(TAG, "data: %s", data.c_str());

  if (data.length() > 0)
  {
    try
    {
      json j = json::parse(data);

      cacheManager.serverHost = j["serverHost"];
      cacheManager.serverPort = j["serverPort"];

      cacheManager.m_username = j["m_username"];
      cacheManager.m_password = j["m_password"];

      cacheManager.scheme = j["scheme"];
      cacheManager.serverIp = j["serverIp"];
      cacheManager.protocol = j["protocol"];
      cacheManager.mainToken = j["mainToken"];

      ESP_LOGI(TAG, "json: %s", j.dump(2).c_str());

      return true;
    }
    catch (const std::exception &e)
    {
      std::cerr << e.what() << '\n';

      /* init config default */
      std::string config_default = this->generateDefaultServerConfig().dump();
      bool flag = this->writeFile(common::CONFIG_PATH_FILE_SERVER_CONFIG, config_default);
      if (flag)
      {
        return true;
      }
    }
  }
  else
  {
    /* init config default */
    std::string config_default = this->generateDefaultServerConfig().dump();
    bool flag = this->writeFile(common::CONFIG_PATH_FILE_SERVER_CONFIG, config_default);
    if (flag)
    {
      return true;
    }
  }

  return false;
}

std::string Storage::serverConfig(void)
{
  json j;
  j["serverHost"] = cacheManager.serverHost;
  j["serverPort"] = cacheManager.serverPort;

  j["m_username"] = cacheManager.m_username;
  j["m_password"] = cacheManager.m_password;
  j["scheme"] = cacheManager.scheme;
  j["serverIp"] = cacheManager.serverIp;
  j["protocol"] = cacheManager.protocol;
  j["mainToken"] = cacheManager.mainToken;

  return j.dump();
}

bool Storage::saveServerConfig(void)
{
  std::string data = this->serverConfig();
  return this->writeFile(common::CONFIG_PATH_FILE_SERVER_CONFIG, data);
}

bool Storage::saveWifiConfig(void)
{
  std::string ssid = cacheManager.ssid;
  std::string password = cacheManager.password;
  bool bssid_set = cacheManager.bssid_set;
  std::vector<uint8_t> bssid_v(cacheManager.sta_bssid, cacheManager.sta_bssid + sizeof(cacheManager.sta_bssid) / sizeof(cacheManager.sta_bssid[0]));

  json j;
  j["ssid"] = ssid;
  j["password"] = password;
  j["bssid"] = bssid_v;
  j["bssid_set"] = cacheManager.bssid_set;
  j["history_station_connected"] = false;

  std::string data = j.dump();

  return this->writeFile(common::CONFIG_PATH_FILE_WIFI_CONFIG, data);
}

bool Storage::loadWifiConfig(void)
{
  std::string data = this->readFile(common::CONFIG_PATH_FILE_WIFI_CONFIG);

  if (data.length() > 0)
  {
    try
    {
      json j = json::parse(data);

      cacheManager.ssid = j["ssid"];
      cacheManager.password = j["password"];
      cacheManager.bssid_set = j["bssid_set"];
      cacheManager.history_station_connected = j["history_station_connected"];

      std::vector<int> bssid = j["bssid"];
      std::copy(bssid.begin(), bssid.end(), cacheManager.sta_bssid);

      ESP_LOGI(TAG, "json: %s", j.dump(2).c_str());

      return true;
    }
    catch (const std::exception &e)
    {
      std::cerr << e.what() << '\n';

      std::string config_default = this->generateDefaultWifiConfig().dump();
      bool flag = this->writeFile(common::CONFIG_PATH_FILE_WIFI_CONFIG, config_default);
      if (flag)
      {
        return true;
      }
    }
  }
  else
  {
    /* init config default */
    std::string config_default = this->generateDefaultWifiConfig().dump();
    bool flag = this->writeFile(common::CONFIG_PATH_FILE_WIFI_CONFIG, config_default);
    if (flag)
    {
      return true;
    }
  }

  return false;
}

void Storage::handleFactory(void *arg)
{
  /* remove config device in files */
  Storage storage;

  json json_wifi = storage.generateDefaultWifiConfig();
  json json_server = storage.generateDefaultServerConfig();
  json _cronjob;

  storage.writeFile(common::CONFIG_PATH_FILE_WIFI_CONFIG, json_wifi.dump());
  storage.writeFile(common::CONFIG_PATH_FILE_SERVER_CONFIG, json_server.dump());

  vTaskDelete(NULL);
}

void Storage::start(void)
{
  ESP_LOGI(TAG, "Storage start.");
  xTaskCreate(&Storage::init, "Storage", 8 * 1024, this, 5, NULL);
}

void Storage::init(void *arg)
{
  Storage *self = (Storage *)arg;
  ESP_LOGI(TAG, "Initializing Storage");

  /* init nvs flash */
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    /* NVS partition was truncated
     * and needs to be erased */
    ret = nvs_flash_erase();

    /* Retry nvs_flash_init */
    ret = nvs_flash_init();
  }

  /* Initialize SPIFFS partition */
  esp_vfs_spiffs_conf_t conf = {
      .base_path = self->_base_path.c_str(),
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true};

  ret = esp_vfs_spiffs_register(&conf);

  if (ret == ESP_OK)
  {
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
      ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    /* load all data to cache */
    self->loadWifiConfig();
    self->loadServerConfig();

    /* get mac device by interface station use esp_mac */
    esp_read_mac(cacheManager.device_mac, ESP_MAC_WIFI_STA);
  }

  /* notify all service */
  // for (auto observer : self->observers)
  // {
  //   self->notify(self->_service, observer.first, new ServicePayload::RecievePayload<ServiceType::StorageEventType>(ServiceType::StorageEventType::EVENT_STORAGE_STARTED, {}));
  // }

  /* start mesh service & log */
  self->notify(self->_service, CentralServices::MESH, new RecievePayload_2<MeshEventType, nullptr_t>(EVENT_MESH_START, nullptr));
  self->notify(self->_service, CentralServices::LOG, new RecievePayload_2<LoggerEventType, nullptr_t>(EVENT_LOGGER_START, nullptr));

  self->_isStarted = true;

  vTaskDelete(NULL);
}

void Storage::onReceive(CentralServices s, void *data)
{
  ESP_LOGI(TAG, "Storage onReceive");
  switch (s)
  {
  case CentralServices::REFACTOR:
  {
    xTaskCreate(this->handleFactory, "handleFactory", 4 * 1024, this, 10, NULL); 
    break;
  }
  case CentralServices::BLUETOOTH:
  {
    RecievePayload_2<StorageEventType, std::map<std::string, std::string>> *payload = static_cast<RecievePayload_2<StorageEventType, std::map<std::string, std::string>> *>(data);

    if (payload->type == EVENT_STORAGE_UPDATE_INFO_WIFI)
    {
      cacheManager.ssid = payload->data["ssid"];
      cacheManager.password = payload->data["password"];
      cacheManager.activeToken = payload->data["token"];

      /* save wifi config */
      this->saveWifiConfig();
      ESP_LOGI(TAG, "WiFi config updated");
    }

    delete payload;
    break;
  }
  default:
    break;
  }
}