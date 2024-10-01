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
    Sensor sensor;

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
    mqtt.registerTwoWayObserver(&sensor, CentralServices::SENSOR);

    button.registerObserver(&led, CentralServices::LED);
    ble.registerObserver(&led, CentralServices::LED);
    mqtt.registerObserver(&factory, CentralServices::REFACTOR);

    factory.registerObserver(&mesh, CentralServices::MESH);
    factory.registerObserver(&storage, CentralServices::STORAGE);

    storage.start(); // run first storage
    button.start();  // run button
    led.start();     // run button
    sensor.start();  // run sensor

    /* keep alive main process */
    while (true)
    {
        vTaskDelay(portMAX_DELAY);
    }
}