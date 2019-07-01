#include "TemperatureService.h"
#include "utils.h"

TemperatureService::TemperatureService()
{

}

void TemperatureService::init(int _pin, DubRtttl* _rtttl)
{
    pin     = _pin;
    rtttl   = _rtttl;
    oneWire = new OneWire(pin);
    DS18B20 = new DallasTemperature(oneWire);

    DS18B20->begin();

    // Loop through each device, save its address
    for (int i = 0; i < DS18B20->getDeviceCount(); i++) {
        // save device's address
        DS18B20->getAddress(addreses[i], i);
    }
}

void TemperatureService::loop()
{
    long now = millis();

    // skip temperature measurement if melody is playing
    if(rtttl->isPlaying()){
        return;
    }

    const int JSON_SIZE = 200;

    if ( now - lastUpdateTime > interval ) { // Take a measurement at a fixed time (tempMeasInterval = 1000ms, 1s)

        for (int i = 0; i < DS18B20->getDeviceCount(); i++) {
            temperatures[i] = this->DS18B20->getTempC( addreses[i] ); // Measuring temperature in Celsius and save the measured value to the array
        }

        
        this->DS18B20->setWaitForConversion(false); //No waiting for measurement
        this->DS18B20->requestTemperatures();       //Initiate the temperature measurement
        lastUpdateTime = millis();                  //Remember the last time measurement

    }
}


float TemperatureService::getTemperatureByAddress(const char * sensorAddress)
{
    int index = this->getDeviceIndex(sensorAddress);
    return temperatures[index];
}

int TemperatureService::getDeviceIndex(const char * sensorAddress){
    for(int i = 0; i < DS18B20->getDeviceCount(); i++){
        if (strcmp(getAddressToString(addreses[i]).c_str(), sensorAddress) == 0){
            return i;
        }
    }
    return 0;
}
