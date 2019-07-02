#pragma once

#include <DHTesp.h>
#include "WebSocketService.h"


class HumidityService {
    public:
        // static instance
        static HumidityService* instance;

        // One wire pin, connected to the sensors
        int                 pin;
        
        // Interval in milliseconds of the data reading
        int                 interval            = 1000;     // 10 seconds by default
        long                lastUpdateTime      = 0;

        float               humidity;
        DHTesp*             dht;


    public:
        HumidityService();
        void    init(int _pin);
        void    loop();
        float   getHumidity();
};