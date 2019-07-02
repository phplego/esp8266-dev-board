#pragma once

#include <Arduino.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include "DubRtttl.h"
#include "Routes.h"
#include "EventService.h"
#include "TemperatureService.h"
#include "HumidityService.h"
#include "DisplayService.h"
#include "MQTTService.h"
#include "WebSocketService.h"
#include "utils.h"

#define ONE_WIRE_BUS        D3                              // Pin to which is attached a temperature sensors DS18B20
#define BUZZER_PIN          D2                              // Pin connected to a speaker
#define DISPLAY_SDA_PIN     D6                              // Display SDA pin
#define DISPLAY_SCL_PIN     D7                              // Display SCL pin

#define MQTT_HOST  "192.168.1.2"    // MQTT host (m21.cloudmqtt.com)
#define MQTT_PORT  11883            // MQTT port (18076)   
#define MQTT_USER  "mfkrdxtb"       // Ingored if brocker allows guest connection
#define MQTT_PASS  "jD-qPTjdtV34"   // Ingored if brocker allows guest connection

class App{

    public:
        WiFiManager*        wifiManager;
        ESP8266WebServer*   server;
        Routes*             routes;
        
        TemperatureService* tempService;
        HumidityService*    humService;
        DisplayService*     dispService;
        MQTTService*        mqttService;
        WebSocketService*   wsService;
        DubRtttl*           rtttl; 

        Adafruit_MQTT_Client*  mqtt;
        Adafruit_MQTT_Publish* mqtt_publish;


    public:
        App();
        void init();
        void loop();
};