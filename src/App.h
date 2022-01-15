#pragma once

#include <Arduino.h>
#include <FS.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include "DubRtttl.h"
#include "Routes.h"
#include "EventService.h"
#include "TemperatureService.h"
#include "HumidityService.h"
#include "DisplayService.h"
#include "MQTTService.h"
#include "WebSocketService.h"
#include "Queue.h"
#include "ChangesDetector.h"

#define ONE_WIRE_BUS        D3                              // Pin to which is attached a temperature sensors DS18B20
#define BUZZER_PIN          D2                              // Pin connected to a speaker
#define DISPLAY_SDA_PIN     D6                              // Display SDA pin
#define DISPLAY_SCL_PIN     D7                              // Display SCL pin


class App{

    public:
        // static instance
        static App* instance;

        WiFiManager*        wifiManager;
        ESP8266WebServer*   server;
        Routes*             routes;
        
        TemperatureService* tempService;
        HumidityService*    humService;
        DisplayService*     dispService;
        MQTTService*        mqttService;
        WebSocketService*   wsService;
        DubRtttl*           rtttl; 
        ChangesDetector<5>* changesDetector;


    public:
        App();
        void init();
        void loop();
};