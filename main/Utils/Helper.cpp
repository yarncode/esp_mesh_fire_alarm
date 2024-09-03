#include "Helper.hpp"

static const char *TAG = "Helper";

double calculateDownloadPercentage(unsigned long long bytesDownloaded, unsigned long long totalBytes)
{
  if (totalBytes == 0)
  {
    return 0.0;
  }

  double percentage = (static_cast<double>(bytesDownloaded) / totalBytes) * 100;

  // Làm tròn kết quả tới 1 chữ số sau dấu thập phân
  return std::round(percentage * 10) / 10.0;
}

static void ip4_to_string(esp_ip4_addr_t ip4, char *str, size_t size)
{
  snprintf(str, size, IPSTR, IP2STR(&ip4));
}

std::string ip4ToString(const esp_ip4_addr_t &ip4)
{
  char str[16];
  ip4_to_string(ip4, str, sizeof(str));
  return std::string(str);
}

std::string macToString(const uint8_t mac[6])
{
  std::ostringstream oss;
  for (int i = 0; i < 6; ++i)
  {
    if (i != 0)
    {
      oss << "_"; // Thêm dấu _ giữa các byte
    }
    // Định dạng từng byte dưới dạng hai chữ số hexa
    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(mac[i]);
  }
  return oss.str();
}

std::string cronReplaceDayOfWeek(std::string str)
{
  std::size_t pos = str.rfind('*');

  std::cout << "pos: " << pos << std::endl;

  if (pos != std::string::npos && pos == str.length() - 1)
  {
    str[pos] = '?';
  }

  return str;
}

esp_err_t mupgrade_version_fallback(void)
{
  esp_err_t ret = ESP_OK;
  const esp_partition_t *partition = NULL;

  partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP,
                                       ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);

  if (partition == NULL)
  {
    partition = esp_ota_get_next_update_partition(NULL);
  }

  ret = esp_ota_set_boot_partition(partition);

  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(TAG, "The next reboot will fall back to the previous version");

  return ESP_OK;
}

bool nvs_set_bool(std::string key, bool value)
{
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  }
  else
  {
    err = nvs_set_u8(nvs_handle, key.c_str(), value);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Error (%s) writing!", esp_err_to_name(err));
    }
    nvs_close(nvs_handle);
    return true;
  }
  return false;
}

bool nvs_get_bool(std::string key)
{
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  }
  else
  {
    uint8_t value = 0;
    err = nvs_get_u8(nvs_handle, key.c_str(), &value);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
    }
    nvs_close(nvs_handle);
    return value;
  }
  return false;
}

bool nvs_set_number(std::string key, int number)
{
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  }
  else
  {
    err = nvs_set_i32(nvs_handle, key.c_str(), number);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Error (%s) writing!", esp_err_to_name(err));
    }
    nvs_close(nvs_handle);
    return true;
  }
  return false;
}

int nvs_get_number(std::string key)
{
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  }
  else
  {
    int32_t number = 0;
    err = nvs_get_i32(nvs_handle, key.c_str(), &number);
    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
    }
    nvs_close(nvs_handle);
    return (int)number;
  }
  return 0;
}

esp_err_t
mdf_info_init(void)
{
  static bool init_flag = false;

  if (!init_flag)
  {
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    init_flag = true;
  }

  return ESP_OK;
}

esp_err_t mdf_info_save(const char *key, const void *value, size_t length)
{
  esp_err_t ret = ESP_OK;
  nvs_handle handle = 0;

  /**< Initialize the default NVS partition */
  mdf_info_init();

  /**< Open non-volatile storage with a given namespace from the default NVS partition */
  ret = nvs_open(MDF_SPACE_NAME, NVS_READWRITE, &handle);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "nvs_open failed (%s)", esp_err_to_name(ret));
    return ret;
  }

  /**< set variable length binary value for given key */
  ret = nvs_set_blob(handle, key, value, length);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "nvs_set_blob failed (%s)", esp_err_to_name(ret));
    return ret;
  }

  /**< Write any pending changes to non-volatile storage */
  nvs_commit(handle);

  /**< Close the storage handle and free any allocated resources */
  nvs_close(handle);

  ESP_LOGI(TAG, "Save key: %s", key);

  return ESP_OK;
}

esp_err_t mdf_info_load(const char *key, void *value, size_t length)
{
  esp_err_t ret = ESP_OK;
  nvs_handle handle = 0;

  /**< Initialize the default NVS partition */
  mdf_info_init();

  /**< Open non-volatile storage with a given namespace from the default NVS partition */
  ret = nvs_open(MDF_SPACE_NAME, NVS_READWRITE, &handle);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "nvs_open failed (%s)", esp_err_to_name(ret));
    return ret;
  }

  /**< get variable length binary value for given key */
  ret = nvs_get_blob(handle, key, value, &length);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "nvs_get_blob failed (%s)", esp_err_to_name(ret));
    return ret;
  }

  /**< Close the storage handle and free any allocated resources */
  nvs_close(handle);

  ESP_LOGI(TAG, "Load key: %s", key);

  return ESP_OK;
}

esp_err_t mdf_info_erase(const char *key)
{
  esp_err_t ret = ESP_OK;
  nvs_handle handle = 0;

  /**< Initialize the default NVS partition */
  mdf_info_init();

  /**< Open non-volatile storage with a given namespace from the default NVS partition */
  ret = nvs_open(MDF_SPACE_NAME, NVS_READWRITE, &handle);

  /**
   * @brief If key is MDF_SPACE_NAME, erase all info in MDF_SPACE_NAME
   */
  if (!strcmp(key, MDF_SPACE_NAME))
  {
    ret = nvs_erase_all(handle);
  }
  else
  {
    ret = nvs_erase_key(handle, key);
  }

  /**< Write any pending changes to non-volatile storage */
  nvs_commit(handle);

  /**< Close the storage handle and free any allocated resources */
  nvs_close(handle);

  ESP_LOGI(TAG, "Erase key: %s", key);

  return ESP_OK;
}

namespace helperString
{

  /* split concept like js */
  std::vector<std::string> split(const std::string &str, char delimiter)
  {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter))
    {
      tokens.push_back(token);
    }
    return tokens;
  }

  /* split concept like js (note: delimiter is string) */
  std::vector<std::string> split(const std::string &str, const std::string &delimiter)
  {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);

    while (end != std::string::npos)
    {
      tokens.push_back(str.substr(start, end - start));
      start = end + delimiter.length();
      end = str.find(delimiter, start);
    }
    tokens.push_back(str.substr(start));

    return tokens;
  }

  std::string genClientId(void)
  {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char *temp;
    int length = asprintf(&temp, "%02X%02X%02X", mac[3], mac[4], mac[5]);
    std::string id = std::string(temp) + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    free(temp);
    return id;
  }

}

namespace chipInfo
{

  uint16_t getChipRevision(void)
  {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    return chip_info.revision;
  }

  uint32_t ESP_getFlashChipId(void)
  {
    uint32_t id = g_rom_flashchip.device_id;
    id = ((id & 0xff) << 16) | ((id >> 16) & 0xff) | (id & 0xff00);
    return id;
  }

  uint8_t getChipCores(void)
  {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    return chip_info.cores;
  }

  std::string getChipModel(void)
  {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    std::string model = "Unknown";

    switch (chip_info.model)
    {
    case CHIP_ESP32:
      model = "ESP32";
      break;
    case CHIP_ESP32S2:
      model = "ESP32-S2";
      break;
    case CHIP_ESP32S3:
      model = "ESP32-S3";
      break;
    case CHIP_ESP32C3:
      model = "ESP32-C3";
      break;
    case CHIP_ESP32H2:
      model = "ESP32-H2";
      break;
    default:
      break;
    }

    return model;
  }

  uint32_t getSketchSize(sketchSize_t response)
  {
    esp_image_metadata_t data;
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (!running)
    {
      return 0;
    }
    const esp_partition_pos_t running_pos = {
        .offset = running->address,
        .size = running->size,
    };
    data.start_addr = running_pos.offset;
    esp_image_verify(ESP_IMAGE_VERIFY, &running_pos, &data);

    if (response)
    {
      return running_pos.size - data.image_len;
    }
    else
    {
      return data.image_len;
    }
  }

  uint32_t getFreeSketchSpace(void)
  {
    const esp_partition_t *_partition = esp_ota_get_next_update_partition(NULL);
    if (!_partition)
    {
      return 0;
    }

    return _partition->size;
  }

  uint32_t getFlashChipSize(void)
  {
    uint32_t id = (ESP_getFlashChipId() >> 16) & 0xFF;
    return 2 << (id - 1);
  }

}