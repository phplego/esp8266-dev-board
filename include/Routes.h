#pragma once

#include <Arduino.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include "DubRtttl.h"
#include "utils.h"



class Routes {
    public:
        WiFiManager*        wifiManager;
        ESP8266WebServer*   server;
        DubRtttl*           rtttl;
    
    public:
        // Default Constructor 
        Routes(WiFiManager*, ESP8266WebServer*, DubRtttl*); 
        void init();

    private:
        bool handleFileRead(String path);
};