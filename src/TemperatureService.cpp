#include "TemperatureService.h"
#include "utils.h"

const char *    TemperatureService::ADDRESS_MAIN            = "282c4445920e0245";
const char *    TemperatureService::ADDRESS_SCND            = "287c004592160207";
const char *    TemperatureService::ADDRESS_PROBE           = "28ee3577911302da";

TemperatureService* TemperatureService::instance = NULL;

TemperatureService::TemperatureService()
{
    instance = this;
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
        DS18B20->getAddress(addresses[i], i);
    }

    this->DS18B20->setWaitForConversion(true);  // Wait for measurement
    this->DS18B20->requestTemperatures();       // Initiate the temperature measurement
    this->ready = true;

}


float TemperatureService::getTemperature(int deviceIndex)
{
    // if(deviceIndex >= this->DS18B20->getDeviceCount() || deviceIndex < 0)
    // {
    //     return 0.0;
    // }

    // return  this->DS18B20->getTempC( addresses[deviceIndex] );
    return temperatures[deviceIndex];
    //return  this->DS18B20->getTempCByIndex( deviceIndex );
}


float TemperatureService::getTemperatureByAddress(const char * sensorAddress)
{
    int index = this->getDeviceIndex(sensorAddress);
    return this->getTemperature(index);
}

int TemperatureService::getDeviceIndex(const char * sensorAddress)
{
    for(int i = 0; i < DS18B20->getDeviceCount(); i++){
        if (strcmp(getAddressToString(addresses[i]).c_str(), sensorAddress) == 0){
            return i;
        }
    }
    return -1;
}

void TemperatureService::loop()
{
    long now = millis();

    // skip temperature measurement if melody is playing
    if(rtttl->isPlaying()){
        return;
    }

    if (!lastUpdateTime || now - lastUpdateTime > interval ) { // Take a measurement at a fixed time (tempMeasInterval = 1000ms, 1s)

        for (int i = 0; i < DS18B20->getDeviceCount(); i++) 
        {
            temperatures[i] = this->DS18B20->getTempCByIndex(i);
            Serial.print(String() +  i + ") " + getAddressToString(addresses[i]) + " = " + getTemperature(i) + " ºC \t");
        }
        Serial.println();

        const int JSON_SIZE = 300;

        DynamicJsonDocument doc(JSON_SIZE);

        // send temperatures as json to web socket
        JsonObject jsonTemperatures = doc.createNestedObject("temperatures");

        for (int i = 0; i < DS18B20->getDeviceCount(); i++) {
            jsonTemperatures[getAddressToString(addresses[i])] = getTemperature(i);
        }

        char jsonStr[JSON_SIZE];
        serializeJson(doc, jsonStr, JSON_SIZE);
        
        WebSocketService::instance->webSocket->broadcastTXT(jsonStr);        

        // request next measurement
        this->DS18B20->setWaitForConversion(false); //No waiting for measurement
        this->DS18B20->requestTemperatures();       //Initiate the temperature measurement
        
        lastUpdateTime = millis();                  //Remember the last time measurement
    }
}
