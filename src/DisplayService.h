#pragma once

#include <SSD1306.h>

#include "TemperatureService.h"
#include "HumidityService.h"


#define DISPLAY_SDA_PIN     D6                              // Display SDA pin
#define DISPLAY_SCL_PIN     D7                              // Display SCL pin


class DisplayService {
    public:

        // static instance
        static DisplayService* instance;

        // Update interval
        int                 interval            = 50;     // redraw interval (milliseconds)
        long                lastUpdateTime      = 0;

        // Display driver object
        SSD1306*            display;

        TemperatureService* tempService;
        HumidityService*    humService;


    public:
        DisplayService();
        void    init(TemperatureService* ts, HumidityService* hs);
        void    loop();
};