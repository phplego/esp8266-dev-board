#pragma once

#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DubRtttl.h"
#include "WebSocketService.h"

#define ONE_WIRE_MAX_DEV    15                              // The maximum number of devices



class TemperatureService {

    private:

        // One wire pin, connected to the sensors
        int                 pin;
        
        // Interval in milliseconds of the data reading
        int                 interval            = 1000;     // 10 seconds by default
        long                lastUpdateTime      = 0;

        // Array of device addresses
        DeviceAddress       addresses[ONE_WIRE_MAX_DEV];     // An array device temperature sensors

        // Array of cached temperatures 
        float               temperatures[ONE_WIRE_MAX_DEV] = {0};     


    public:
        static TemperatureService* instance;
        static const char *    ADDRESS_MAIN;
        static const char *    ADDRESS_SCND;
        static const char *    ADDRESS_PROBE;

        
        // Is the first measurement done
        bool                ready               = false;    


        OneWire*            oneWire;
        DallasTemperature*  DS18B20;
        DubRtttl*           rtttl;


    public:
        TemperatureService();
        void    init(int _pin, DubRtttl* _rtttl);
        void    loop();
        int     getDeviceCount();
        int     getDeviceIndex(const char * sensorAddress);
        float   getTemperature(int deviceIndex);
        float   getTemperatureByAddress(const char * sensorAddress);
};


