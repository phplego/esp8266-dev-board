#include "MQTTService.h"


MQTTService::MQTTService()
{

}

void MQTTService::init()
{
    // Create an ESP8266 WiFiClient class to connect to the MQTT server.
    WiFiClient* client = new WiFiClient();

    // Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
    mqtt            = new Adafruit_MQTT_Client(client, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS);
    mqtt_publish    = new Adafruit_MQTT_Publish(mqtt, "wifi2mqtt/esp8266_board_1");

}

/**
 * Conect to MQTT brocker. Do nothing if already connected
 */
void MQTTService::connect() {
  int8_t ret;

  // Do nothing if already connected.
  if (this->mqtt->connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 10;
  while ((ret = this->mqtt->connect()) != 0) { // connect will return 0 for connected
       Serial.println(this->mqtt->connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       this->mqtt->disconnect();
       delay(3000);  // wait 3 seconds
       retries--;
       if (retries == 0) {
         return;
       }
  }
  Serial.println("MQTT Connected!");
}



void MQTTService::loop()
{
    const int JSON_SIZE = 300;
    long now = millis();
    
    if ( (!lastUpdateTime && TemperatureService::instance->temperatures[0] > 0) || now - lastUpdateTime > interval ) {

        StaticJsonBuffer<JSON_SIZE> jsonBuffer;
        //JsonArray& others = root.createNestedArray("others");
        
        JsonObject& root = jsonBuffer.createObject();
        
        root["humidity"]  = HumidityService::instance->getHumidity();
        root["memory"]    = system_get_free_heap_size();
        root["time"]      = now / 1000;

        JsonObject& temperatures = root.createNestedObject("temperatures");
        
        temperatures["main"]    = TemperatureService::instance->getTemperatureByAddress(TemperatureService::ADDRESS_MAIN);
        temperatures["second"]  = TemperatureService::instance->getTemperatureByAddress(TemperatureService::ADDRESS_SCND);

        char jsonStr[JSON_SIZE];
        root.prettyPrintTo(jsonStr, JSON_SIZE);

        // It's important to connect before publish!
        this->connect();

        if (!this->mqtt_publish->publish(jsonStr)) {
          Serial.println("MQTT publish failed!");
        } else {
          Serial.println("MQTT publish ok!");
        }
        
        lastUpdateTime = millis();
    }
}
