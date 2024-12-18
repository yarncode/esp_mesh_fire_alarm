#ifndef CONST_TYPE_H
#define CONST_TYPE_H

#include <any>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "driver/gpio.h"

#include "sdkconfig.h"

enum CentralServices {
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

typedef struct {
  gpio_num_t gpio;
  std::string name;
} gpio_custom_t;

namespace common {
static const std::string CONFIG_PATH_FILE_WIFI_CONFIG = "/config_wifi.txt";
static const std::string CONFIG_PATH_FILE_SERVER_CONFIG = "/config_server.txt";
static const std::string CONFIG_PATH_FILE_THRESHOLD_CONFIG =
    "/config_trigger.txt";
#ifdef CONFIG_MODE_GATEWAY
static const std::string CONFIG_NODE_TYPE_DEVICE = "GATEWAY";

/* number gpio input */
static const int CONFIG_GPIO_INPUT_COUNT = 6;
/* number gpio output */
static const int CONFIG_GPIO_OUTPUT_COUNT = 3;

static const std::map<int, gpio_custom_t> CONFIG_GPIO_INPUT = {
    {0, {GPIO_NUM_36, "input_0"}}, {1, {GPIO_NUM_39, "input_1"}},
    {2, {GPIO_NUM_34, "input_2"}}, {3, {GPIO_NUM_35, "input_3"}},
    {4, {GPIO_NUM_32, "input_4"}}, {5, {GPIO_NUM_33, "input_5"}},
};

static const std::map<int, gpio_custom_t> CONFIG_GPIO_OUTPUT = {
    {0, {GPIO_NUM_25, "output_0"}},
    {1, {GPIO_NUM_26, "output_1"}},
    {2, {GPIO_NUM_27, "output_2"}},
};

#elif CONFIG_MODE_NODE
static const std::string CONFIG_NODE_TYPE_DEVICE = "NODE";

/* number gpio input */
static const int CONFIG_GPIO_INPUT_COUNT = 1;
/* number gpio output */
static const int CONFIG_GPIO_OUTPUT_COUNT = 1;

static const std::map<int, gpio_custom_t> CONFIG_GPIO_INPUT = {
    {0, {GPIO_NUM_36, "input_0"}},
};

static const std::map<int, gpio_custom_t> CONFIG_GPIO_OUTPUT = {
    {0, {GPIO_NUM_25, "output_0"}},
};
#endif
static const int CONFIG_PORT = 5000;
static const std::string CONFIG_SERVER_IP = "127.0.0.1";
static const std::string CONFIG_SERVER_HOST = "localhost";
static const std::string CONFIG_SERVER_PROTOCOL = "http";

/* -1: no limit, -2: disabled */
static const std::vector<int> CONFIG_THRESHOLD_TEMP = std::vector<int>{50, -1};
static const std::vector<int> CONFIG_THRESHOLD_HUMI = std::vector<int>{-2, -2};
static const std::vector<int> CONFIG_THRESHOLD_SMOKE =
    std::vector<int>{500, -1};

static const std::string CONFIG_NOTIFY_SYNC_THRESHOLD = "SYNC_THRESHOLD";
static const std::string CONFIG_NOTIFY_SYNC_GPIO = "SYNC_GPIO";
static const std::string CONFIG_NOTIFY_ACK_PAYLOAD = "ACK_PAYLOAD";
static const std::string CONFIG_NOTIFY_REMOVE_DEVICE = "REMOVE_DEVICE";

static const std::string CONFIG_KEY_INPUT = "input";
static const std::string CONFIG_KEY_OUTPUT = "output";

static const std::string CONFIG_CONTROL_TYPE_GPIO = "gpio";
static const std::string CONFIG_CONTROL_TYPE_BUZZER = "buzzer";

static const std::string CONFIG_CONTROL_MODE_ALL_GPIO = "all";
static const std::string CONFIG_CONTROL_MODE_MULTIPLE_GPIO = "mutiple";
static const std::string CONFIG_CONTROL_MODE_SINGLE_GPIO = "single";

static const std::string CONFIG_MESH_CODE_SET_BUZZER = "SET_BUZZER";
static const std::string CONFIG_MESH_CODE_SET_RELAY = "SET_RELAY";

} // namespace common

#endif // CONST_TYPE_H
