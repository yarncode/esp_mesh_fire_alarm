#include "Sensor.hpp"
#include "Buzzer.hpp"
#include "Cache.h"
#include "Mesh.hpp"
#include "Mqtt.hpp"
#include "Relay.hpp"

#include "MQSensor.h"

#ifdef CONFIG_MODE_NODE
#include "sht31.h"
#endif

#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

using namespace nlohmann;

using namespace ServiceType;
using namespace ServicePayload;

static const char *TAG = "Sensor";
static TaskHandle_t _taskSampleValue = NULL;
static TaskHandle_t _taskInputValue = NULL;

/* sht31 */

/* mq2 */
MQUnifiedsensor MQ2("ESP32", 5, 12, GPIO_NUM_34, "MQ-2");
static std::map<gpio_num_t, int> _gb_gpio_pos;
static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg) {
  uint32_t gpio_num = (uint32_t)arg;
  if (gpio_evt_queue != NULL) {
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
  }
}

void Sensor::sampleValue(void *arg) {
  Sensor *self = static_cast<Sensor *>(arg);
  Buzzer *buzzer =
      static_cast<Buzzer *>(self->getObserver(CentralServices::BUZZER));
  float temperature, humidity;
  double smoke;
  json body;
  bool flag_warning = false;
  time_t time_now;
  time_t time_last = 0;
  bool flagSensorWarning = false;

  while (true) {
    /* reset state warning */
    flagSensorWarning = false;

/* update data sensor MQ2 */
// MQ2.update();

/* get temperature and humidity from sht31 */
#ifdef CONFIG_MODE_NODE

    sht31_readTempHum();

    humidity = sht31_readHumidity();
    temperature = sht31_readTemperature();

#endif

    smoke = analogRead(GPIO_NUM_34) * 4;

    ESP_LOGI(TAG, "Sensor sampleValue: %f", smoke);

    time_now =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    /* get smoke from data */
    // smoke = MQ2.readSensor();

    body["smoke"] = std::isnan(smoke) || std::isinf(smoke) ? 0.0 : smoke;

#ifdef CONFIG_MODE_NODE

    body["temperature"] =
        std::isnan(temperature) ? 0.0 : std::ceil(temperature * 100.0) / 100.0;
    body["humidity"] =
        std::isnan(humidity) ? 0.0 : std::ceil(humidity * 100.0) / 100.0;

#endif
    /* log temp, humi, smoke */
    // ESP_LOGI(TAG, "Sensor sampleValue: %f, %f, %f", temperature, humidity,
    // smoke);
    ESP_LOGI(TAG, "Sensor sampleValue: %s", body.dump(2).c_str());

    self->notify(self->_service, CentralServices::MQTT,
                 new RecievePayload_2<SensorType, std::string>(
                     SENSOR_SEND_SAMPLE_DATA, body.dump()));

/* check warning */
#ifdef CONFIG_MODE_NODE

    if (temperature > cacheManager.thresholds_temp[0]) {
      flagSensorWarning = true;
    }

    if (smoke > cacheManager.thresholds_smoke[0]) {
      flagSensorWarning = true;
    }

#endif

    if (self->flagWarning || flagSensorWarning ||
        gpio_get_level(common::CONFIG_GPIO_INPUT.at(0).gpio) == 0) {
      /* debounce for 10 seconds */
      if (buzzer->stateWarning() == false && time_now - time_last > 10) {
        buzzer->startWarning(true);
        time_last = time_now;
      }
    } else {
      /* check state warning & stop */
      buzzer->stopWarning(true);
    }

    /* get value every 5 seconds */
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }

  vTaskDeleteWithCaps(NULL);
}

void Sensor::gpioInterrupt(void *arg) {

  Sensor *self = static_cast<Sensor *>(arg);
  uint32_t io_num;
  json _body;
  Mqtt *mqtt = static_cast<Mqtt *>(self->getObserver(CentralServices::MQTT));
#ifdef CONFIG_MODE_NODE
  Buzzer *buzzer =
      static_cast<Buzzer *>(self->getObserver(CentralServices::BUZZER));
#endif
  Relay *relay =
      static_cast<Relay *>(self->getObserver(CentralServices::RELAY));
  Mesh *mesh = static_cast<Mesh *>(self->getObserver(CentralServices::MESH));

  while (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY) == pdPASS) {

    gpio_intr_disable((gpio_num_t)io_num);

    try {

      int pos = _gb_gpio_pos.find((gpio_num_t)io_num) != _gb_gpio_pos.end()
                    ? _gb_gpio_pos[(gpio_num_t)io_num]
                    : -1;
      bool state = gpio_get_level((gpio_num_t)io_num) == 1 ? false : true;

      if (pos >= 0) {
        ESP_LOGI(TAG, "-> [gpio]-[%d]-[%s]", (int)io_num,
                 state ? "HIGH" : "LOW");

#ifdef CONFIG_MODE_NODE
        if (buzzer->stateWarning() == false && state == true) {
          // buzzer->startWarning(true);
          relay->setStateRelay(0, true, false, true, true);
        } else {
          // buzzer->stopWarning(true);
          relay->setStateRelay(0, false, false, true, true);
        }
#endif

        ESP_LOGI(TAG, "Sensor gpioInterrupt: %d, %d", pos, state);

#ifdef CONFIG_MODE_GATEWAY
        relay->setStateRelay(pos, state, false, true);
#endif

        if (state != cacheManager.input_state[pos]) {
          _body["timestamp"] = std::chrono::system_clock::to_time_t(
              std::chrono::system_clock::now());
          _body["state"] = state;
          _body["pos"] = pos;
          _body["type"] = common::CONFIG_KEY_INPUT;
          _body["mode"] = common::CONFIG_CONTROL_MODE_SINGLE_GPIO;

          /* send to broker */
          mqtt->sendMessage(ChannelMqtt::CHANNEL_IO_INPUT, _body.dump());
        }
      }

      cacheManager.input_state[pos] = state;

    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
    }

    gpio_intr_enable((gpio_num_t)io_num);
  }

  vTaskDeleteWithCaps(NULL);
}

void Sensor::start(void) {
  ESP_LOGI(TAG, "Sensor start");
  xTaskCreateWithCaps(&Sensor::init, "Sensor::init", 4 * 1024, this, 5, NULL,
                      MALLOC_CAP_SPIRAM);
}

void Sensor::stop(void) {
  ESP_LOGI(TAG, "Sensor stop");
  xTaskCreateWithCaps(&Sensor::deinit, "Sensor::deinit", 4 * 1024, this, 5,
                      NULL, MALLOC_CAP_SPIRAM);
}

void Sensor::init(void *arg) {
  Sensor *self = static_cast<Sensor *>(arg);

  /* setup gpio input */
  int index = 0;
  for (auto it = common::CONFIG_GPIO_INPUT.begin();
       it != common::CONFIG_GPIO_INPUT.end(); it++) {
    gpio_num_t pin = it->second.gpio;

    ESP_LOGI(TAG, "install input pin: %d", pin);

    gpio_config_t io_conf;

    memset(&io_conf, 0, sizeof(gpio_config_t));

    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    // bit mask of the pins
    io_conf.pin_bit_mask = (1ULL << pin);
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(pin, gpio_isr_handler, (void *)pin);

    /* init map pos gpio */
    _gb_gpio_pos[pin] = index;

    /* read value */
    cacheManager.input_state[index] = gpio_get_level(pin) ? false : true;
    ESP_LOGI(TAG, "input state: %d", cacheManager.input_state[index]);

    index++;
  }

  gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));

  if (gpio_evt_queue != NULL) {
    xTaskCreateWithCaps(&Sensor::gpioInterrupt, "Sensor::gpioInterrupt",
                        5 * 1024, self, 6, &_taskInputValue, MALLOC_CAP_SPIRAM);
  }

#ifdef CONFIG_MODE_NODE
  /* Initialize sht31 */
  sht31_init();
#endif

  // ESP_LOGI(TAG, "Calibrate done.");
#ifdef CONFIG_MODE_NODE

  ESP_LOGI(TAG, "Sensor init");
  xTaskCreateWithCaps(&Sensor::sampleValue, "Sensor::sampleValue", 4 * 1024,
                      self, 5, &_taskSampleValue, MALLOC_CAP_SPIRAM);

#endif

  vTaskDeleteWithCaps(NULL);
}

void Sensor::deinit(void *arg) {
  Sensor *self = static_cast<Sensor *>(arg);

  ESP_LOGI(TAG, "Sensor deinit");
#ifdef CONFIG_MODE_NODE

  if (_taskSampleValue != NULL) {
    vTaskDeleteWithCaps(_taskSampleValue);
    _taskSampleValue = NULL;
  }

#endif

  if (_taskInputValue != NULL) {
    vTaskDeleteWithCaps(_taskInputValue);
    _taskInputValue = NULL;
  }

  vTaskDeleteWithCaps(NULL);
}

void Sensor::onReceive(CentralServices s, void *data) {
  ESP_LOGI(TAG, "Sensor onReceive: %d", s);
}