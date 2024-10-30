#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "Storage.hpp"
#include "Mesh.hpp"
#include "Log.hpp"
#include "Mqtt.hpp"
#include "Bluetooth.hpp"
#include "Refactor.hpp"
#include "Button.hpp"
#include "Led.hpp"
#include "ApiCaller.hpp"
#include "Sensor.hpp"
#include "SNTP.hpp"
#include "Relay.hpp"

#ifdef CONFIG_MODE_GATEWAY
#include "Screen.hpp"
#endif

#ifdef CONFIG_MODE_NODE
#include "Buzzer.hpp"
#endif

extern "C" void app_main(void)
{
    Storage storage;
    Mesh mesh;
    Log logger;
    Mqtt mqtt;
    Bluetooth ble;
    Refactor factory;
    Button button;
    Led led;
    ApiCaller api;
    Sntp sntp;
    Relay relay;

#ifdef CONFIG_MODE_GATEWAY
    LCDScreen screen;
#endif

#ifdef CONFIG_MODE_NODE
    Sensor sensor;
    Buzzer buzzer;
#endif

    /* init storage */
    storage.registerTwoWayObserver(&mesh, CentralServices::MESH);
    storage.registerTwoWayObserver(&logger, CentralServices::LOG);
    storage.registerTwoWayObserver(&ble, CentralServices::BLUETOOTH);
    mesh.registerTwoWayObserver(&mqtt, CentralServices::MQTT);
    button.registerTwoWayObserver(&ble, CentralServices::BLUETOOTH);
    logger.registerObserver(&mesh, CentralServices::MESH);
    ble.registerTwoWayObserver(&mesh, CentralServices::MESH);
    ble.registerTwoWayObserver(&mqtt, CentralServices::MQTT);
    api.registerTwoWayObserver(&mqtt, CentralServices::MQTT);
    mesh.registerTwoWayObserver(&api, CentralServices::API_CALLER);
    mesh.registerTwoWayObserver(&sntp, CentralServices::SNTP);
    mqtt.registerTwoWayObserver(&relay, CentralServices::RELAY);

#ifdef CONFIG_MODE_NODE
    mqtt.registerTwoWayObserver(&sensor, CentralServices::SENSOR);
    mqtt.registerTwoWayObserver(&buzzer, CentralServices::BUZZER);
    buzzer.registerTwoWayObserver(&sensor, CentralServices::SENSOR);
#endif

    button.registerObserver(&led, CentralServices::LED);
    ble.registerObserver(&led, CentralServices::LED);
    mqtt.registerObserver(&factory, CentralServices::REFACTOR);

    factory.registerObserver(&mesh, CentralServices::MESH);
    factory.registerObserver(&storage, CentralServices::STORAGE);

    storage.start(); // run first storage
    button.start();  // run button
    relay.start();   // run lcd
    led.start();     // run button

#ifdef CONFIG_MODE_NODE
    sensor.start();  // run sensor
#endif

#ifdef CONFIG_MODE_GATEWAY
    // screen.start();    // run lcd
#endif

#ifdef CONFIG_MODE_NODE
    buzzer.start();
#endif

    /* keep alive main process */
    while (true)
    {
        vTaskDelay(portMAX_DELAY);
    }
}