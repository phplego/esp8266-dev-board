#pragma once

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include "DubRtttl.h"
#include "utils.h"



class Routes {
    public:
        ESP8266WebServer*   server;
        DubRtttl*           rtttl;
    
    public:
        // Default Constructor 
        Routes(ESP8266WebServer*, DubRtttl*); 

    private:
        bool handleFileRead(String path);
};