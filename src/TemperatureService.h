#include <OneWire.h>
#include <DallasTemperature.h>

class TemperatureService {
    public:
        int                 pin;
        OneWire*            oneWire;
        DallasTemperature*  DS18B20;

    public:
        TemperatureService();
        void init(int _pin);
        int getDeviceCount();
        float getTemperature(char * sensorId);

};