#pragma once

#include <Arduino.h>
#include <ESP8266WebServer.h>


class Routes {
    public:
        ESP8266WebServer* server;
    
    public:
        // Default Constructor 
        Routes(ESP8266WebServer*); 
};