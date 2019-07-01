#include <OneWire.h>
#include <DallasTemperature.h>
#include "DubRtttl.h"

#define ONE_WIRE_MAX_DEV    15                              // The maximum number of devices


class TemperatureService {
    public:
        // One wire pin, connected to the sensors
        int                 pin;
        
        // Interval in milliseconds of the data reading
        int                 interval            = 1000;     // 10 seconds by default
        long                lastUpdateTime      = 0;

        // Array of device addresses
        DeviceAddress       addreses[ONE_WIRE_MAX_DEV];     // An array device temperature sensors

        // Array of measured values
        float               temperatures[ONE_WIRE_MAX_DEV]; // Saving the last measurement of temperature
        


        OneWire*            oneWire;
        DallasTemperature*  DS18B20;
        DubRtttl*           rtttl;


    public:
        TemperatureService();
        void    init(int _pin, DubRtttl* _rtttl);
        void    loop();
        int     getDeviceCount();
        int     getDeviceIndex(const char * sensorAddress);
        float   getTemperatureByAddress(const char * sensorAddress);
};