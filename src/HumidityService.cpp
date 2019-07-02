#include "HumidityService.h"
#include "utils.h"

HumidityService* HumidityService::instance = NULL;

HumidityService::HumidityService()
{
    instance = this;
}

void HumidityService::init(int _pin)
{
    pin     = _pin;

    // Setup DHT-11 humidity sensor
    dht = new DHTesp();
    dht->setup(pin, DHTesp::DHT11); // Connect DHT sensor to GPIO 4 

}

void HumidityService::loop()
{
    long now = millis();

    if ( now - lastUpdateTime > interval ) { // Take a measurement every interval 
        // Read data
        float h = dht->getHumidity();
        if(dht->getStatus() != 0){
            return;
        }
        humidity = h;
        //float temperature = dht->getTemperature();
        //float heatIndex = dht->computeHeatIndex(temperature, humidity, false);
        
        //Remember the last time measurement
        lastUpdateTime = millis();                  
    }
}


float HumidityService::getHumidity()
{
    return humidity;
}