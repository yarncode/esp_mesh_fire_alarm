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
    Sensor sensor;

#ifdef CONFIG_MODE_GATEWAY
    LCDScreen screen;
#endif

#ifdef CONFIG_MODE_NODE
    Buzzer buzzer;
#endif

    /* init storage */
    storage.registerTwoWayObserver(&mesh, CentralServices::MESH);
    storage.registerTwoWayObserver(&logger, CentralServices::LOG);
    storage.registerTwoWayObserver(&ble, CentralServices::BLUETOOTH);
    storage.registerTwoWayObserver(&relay, CentralServices::RELAY);
    storage.registerTwoWayObserver(&button, CentralServices::BUTTON);
    storage.registerTwoWayObserver(&led, CentralServices::LED);
    mesh.registerTwoWayObserver(&mqtt, CentralServices::MQTT);
    button.registerTwoWayObserver(&ble, CentralServices::BLUETOOTH);
    button.registerTwoWayObserver(&mesh, CentralServices::MESH);
    ble.registerTwoWayObserver(&mesh, CentralServices::MESH);
    ble.registerTwoWayObserver(&mqtt, CentralServices::MQTT);
    api.registerTwoWayObserver(&mqtt, CentralServices::MQTT);
    ble.registerTwoWayObserver(&api, CentralServices::API_CALLER);
    mesh.registerTwoWayObserver(&api, CentralServices::API_CALLER);
    mesh.registerTwoWayObserver(&sntp, CentralServices::SNTP);
    mqtt.registerTwoWayObserver(&relay, CentralServices::RELAY);
    mqtt.registerTwoWayObserver(&sensor, CentralServices::SENSOR);

#ifdef CONFIG_MODE_GATEWAY
    screen.registerTwoWayObserver(&mqtt, CentralServices::MQTT);
#endif

#ifdef CONFIG_MODE_NODE
    mqtt.registerTwoWayObserver(&buzzer, CentralServices::BUZZER);
    buzzer.registerTwoWayObserver(&sensor, CentralServices::SENSOR);
#endif

    mesh.registerObserver(&led, CentralServices::LED);
    logger.registerObserver(&mesh, CentralServices::MESH);
    button.registerObserver(&led, CentralServices::LED);
    ble.registerObserver(&led, CentralServices::LED);
    mqtt.registerObserver(&factory, CentralServices::REFACTOR);

    factory.registerObserver(&mesh, CentralServices::MESH);
    factory.registerObserver(&storage, CentralServices::STORAGE);

    storage.start(); // run first storage
    sensor.start();  // run sensor

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