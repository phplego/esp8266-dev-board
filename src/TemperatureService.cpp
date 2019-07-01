#include "TemperatureService.h"

TemperatureService::TemperatureService()
{

}

void TemperatureService::init(int _pin)
{
    pin = _pin;
    oneWire = new OneWire(pin);
    DS18B20 = new DallasTemperature(oneWire);
}