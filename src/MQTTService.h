#pragma once

#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <user_interface.h>         //for getting free memory size
#include "TemperatureService.h"
#include "HumidityService.h"


#define MQTT_HOST  "192.168.1.157"  // MQTT host (m21.cloudmqtt.com)
#define MQTT_PORT  11883            // MQTT port (18076)   
#define MQTT_USER  "mfkrdxtb"       // Ingored if brocker allows guest connection
#define MQTT_PASS  "jD-qPTjdtV34"   // Ingored if brocker allows guest connection


class MQTTService {
    public:
        // Interval in milliseconds of the data publishing
        int                     interval            = 60*1000;     // MQTT publish interval
        long                    lastUpdateTime      = 0;

        // MQTT library objects
        Adafruit_MQTT_Client*   mqtt;
        Adafruit_MQTT_Publish*  mqtt_publish;

    public:
        MQTTService();
        void    init();
        void    connect();
        void    publishState();
        void    loop();
};

