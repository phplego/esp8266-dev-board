#include <Arduino.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHTesp.h>
#include <WebSocketsServer.h>
#include <WiFiManager.h>
#include <SSD1306.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include "ArduinoJson.h"
#include "DubRtttl.h"
#include "utils.h"
#include "Routes.h"
#include "App.h"



long            lastMqttPublishTime             = 0;        // The last MQTT publish time
const long      mqttPublishInterval             = 60*1000;  // MQTT publish interval

const char *    TEMP_ID_MAIN                    = "28ee3577911302da";
const char *    TEMP_ID_SCND                    = "287c004592160207";



// Application object
App app1 = App();



void myTone(int freq, int duration)
{
    tone(BUZZER_PIN, freq, duration);
    delay(duration);
}


void getTempStr(char * buffer, const char * sensorAddress){
    if(app1.tempService->DS18B20->getDeviceCount() == 0){
        sprintf(buffer, "no devices");
        return;
    }
    sprintf(buffer, "%.1f", app1.tempService->getTemperatureByAddress(sensorAddress));
}

// void humidityLoop() {
//     //delay(dht.getMinimumSamplingPeriod());

//     float h = app1.dht->getHumidity();
//     float temperature = app1.dht->getTemperature();

//     if(app1.dht->getStatus() != 0){
//         return;
//     }
//     theHumidity = h;
//     float heatIndex = app1.dht->computeHeatIndex(temperature, theHumidity, false);

//     Serial.print(app1.dht->getStatusString());
//     Serial.print("\t humidity: ");
//     Serial.print(theHumidity, 1);
//     Serial.print("\t t: ");
//     Serial.print(temperature, 1);
//     Serial.print("\t HeatIndex: ");
//     Serial.print(heatIndex, 1);
//     Serial.print("\t Free heap size: ");
//     Serial.println(system_get_free_heap_size(), 1);

//     char jsonStr[100];
//     sprintf(jsonStr, "{\"hum\":%.0f, \"dht_temp\":%.1f, \"heat_index\":%.1f}", theHumidity, temperature, heatIndex);
//     app1.webSocket->broadcastTXT(jsonStr);
// }


// //Loop measuring the temperature
// void tempLoop(long now) {

//     // skip temperature measurement if melody is playing
//     if(app1.rtttl->isPlaying()){
//         return;
//     }

//     const int JSON_SIZE = 200;

//     if ( now - lastTempMeasTime > tempMeasInterval ) { // Take a measurement at a fixed time (tempMeasInterval = 1000ms, 1s)
//         StaticJsonBuffer<JSON_SIZE> jsonBuffer;
//         JsonObject& root = jsonBuffer.createObject();
//         JsonObject& temperatures = root.createNestedObject("temperatures");

//         for (int i = 0; i < numberOfDevices; i++) {
//             tempDev[i] = app1.tempService->DS18B20->getTempC( devAddr[i] ); // Measuring temperature in Celsius and save the measured value to the array
//             temperatures[getAddressToString(devAddr[i])] = tempDev[i];
//         }

//         char jsonStr[JSON_SIZE];
//         root.prettyPrintTo(jsonStr, JSON_SIZE);
//         app1.webSocket->broadcastTXT(jsonStr);
        
//         app1.tempService->DS18B20->setWaitForConversion(false); //No waiting for measurement
//         app1.tempService->DS18B20->requestTemperatures(); //Initiate the temperature measurement
//         lastTempMeasTime = millis();  //Remember the last time measurement

//         humidityLoop();
//     }
// }



void onAPStarted(WiFiManager * manager){
    app1.dispService->display->clear();
    app1.dispService->display->setFont(ArialMT_Plain_10);
    app1.dispService->display->setTextAlignment(TEXT_ALIGN_LEFT);
    int top = 20;
    app1.dispService->display->drawString(0, top + 0, String("Please connect to Wi-Fi"));
    app1.dispService->display->drawString(0, top + 10, String("Network: E-STOVE"));
    app1.dispService->display->drawString(0, top + 20, String("Password: 12341234"));
    app1.dispService->display->drawString(0, top + 30, String("Then go to ip: 10.0.1.1"));
    app1.dispService->display->display();
}



//------------------------------------------
void setup() {

    // Play first start melody
    myTone(800, 100);
    myTone(400, 100);
    myTone(1200, 100);

    app1.init();

    myTone(600, 100);
    myTone(300, 100);


    // Setup Serial port speed
    Serial.begin(115200);

    // Setup FileSystem
    SPIFFS.begin();


    // Setup display
    app1.dispService->display->init();
    app1.dispService->display->flipScreenVertically();
    app1.dispService->display->clear();
    app1.dispService->display->setFont(ArialMT_Plain_16);


    // Setup WIFI
    app1.dispService->display->drawString(20,20, "Hello Oleg!");
    app1.dispService->display->setFont(ArialMT_Plain_10);
    app1.dispService->display->drawString(20,50, "connecting to wifi..");
    app1.dispService->display->display();
    app1.wifiManager->setAPCallback(onAPStarted);
    app1.wifiManager->setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    app1.wifiManager->autoConnect("E-STOVE", "12341234"); // Blocks execution. Waits until connected

    // Wait for WIFI connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());


    // Setup routes

    app1.server->on("/logout", [](){
        if(app1.server->method() == HTTP_POST){
            app1.server->send(200, "text/html", "OK");
            app1.wifiManager->resetSettings();
        }
        else{
            app1.server->send(400, "text/html", "post method only");
        }
    });
    app1.server->begin();
    Serial.println("HTTP server started at ip " + WiFi.localIP().toString() );


    // Setup Web Socket
    app1.webSocket->begin();                          // start the websocket server
    app1.webSocket->onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t len){
        switch (type) {
            case WStype_DISCONNECTED:             // if the websocket is disconnected
                Serial.printf("[%u] Disconnected!\n", num);
                break;
            case WStype_CONNECTED: {              // if a new websocket connection is established
                IPAddress ip = app1.webSocket->remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            }
                break;
            case WStype_TEXT:                     // if new text data is received
                Serial.printf("[%u] get Text: %s\n", num, payload);
                if (payload[0] == '#') {            // we get RGB data
                    // example
                }
                app1.webSocket->sendTXT(num, payload, len);
                break;
        }
    });

    // Setup Logging web socket
    app1.logSocket->begin();                          // start the websocket server (for logging)
    app1.logSocket->onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t len){
        switch (type) {
            case WStype_DISCONNECTED:             // if the websocket is disconnected
                Serial.printf("[%u] Disconnected!\n", num);
                break;
            case WStype_CONNECTED: {              // if a new websocket connection is established
                IPAddress ip = app1.logSocket->remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            }
                break;
            case WStype_TEXT:                     // if new text data is received
                Serial.printf("[%u] get Text: %s\n", num, payload);
                char str[256];
                sprintf(str, "[%u] get Text: %s\n", num, payload);
                app1.logSocket->sendTXT(num, str);
                break;
        }        
    });


    // Play second start melody
    myTone(800, 100);
    myTone(400, 100);
    myTone(1200, 100);

    myTone(800, 100);
    myTone(400, 100);
    myTone(1200, 100);


    Serial.println("*** end setup ***");
}




void loop() 
{
    // Main application loop
    app1.loop();
}