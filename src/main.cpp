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



// #define ONE_WIRE_MAX_DEV    15                              // The maximum number of devices
// #define ONE_WIRE_BUS        D3                              // Pin to which is attached a temperature sensors DS18B20
// #define MELODY_PIN          D2                              // Pin connected to a speaker
// #define DISPLAY_SDA_PIN     D6                              // Display SDA pin
// #define DISPLAY_SCL_PIN     D7                              // Display SCL pin


//DubRtttl          rtttl(MELODY_PIN);                        // Melody player
//WiFiManager       wifiManager;                              // WiFi Manager
//OneWire           oneWire(ONE_WIRE_BUS);                    
//DallasTemperature DS18B20(&oneWire);                        // Temperature sensor access object    
//DHTesp            dht;                                      // DHT-11

int             numberOfDevices;                            // Number of temperature devices found
DeviceAddress   devAddr[ONE_WIRE_MAX_DEV];                  // An array device temperature sensors
float           tempDev[ONE_WIRE_MAX_DEV];                  // Saving the last measurement of temperature
float           tempDevLast[ONE_WIRE_MAX_DEV];              // Previous temperature measurement
float           theHumidity                     = 0;        // Humidity
long            lastTempMeasTime;                           // The last measurement time
const int       tempMeasInterval                = 1000;     // The frequency of temperature measurement
long            lastDispTime;                               // The last display update time
const int       updateDisplayInterval           = 50;       // The frequency of display refreshing
long            loopCount                       = 0;
int             warnLevel                       = 0;

long            lastMqttPublishTime             = 0;        // The last MQTT publish time
const long      mqttPublishInterval             = 60*1000;  // MQTT publish interval

const char *    TEMP_ID_MAIN                    = "28ee3577911302da";
const char *    TEMP_ID_SCND                    = "287c004592160207";



//------------------------------------------
// HTTP
//ESP8266WebServer server(80);
//Routes routes(&server);
// WebSocketsServer webSocket(81);
// WebSocketsServer logSocket(82);



//------------------------------------------
// DISPLAY
//SSD1306  display (0x3c, DISPLAY_SDA_PIN, DISPLAY_SCL_PIN);


//------------------------------------------
// MQTT

// #define MQTT_HOST  "192.168.1.2"    // MQTT host (m21.cloudmqtt.com)
// #define MQTT_PORT  11883            // MQTT port (18076)   
// #define MQTT_USER  "mfkrdxtb"       // Ingored if brocker allows guest connection
// #define MQTT_PASS  "jD-qPTjdtV34"   // Ingored if brocker allows guest connection

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
// WiFiClient client;

// // Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
// Adafruit_MQTT_Client mqtt(&client, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS);

// // Publish object
// Adafruit_MQTT_Publish mqtt_publish = Adafruit_MQTT_Publish(&mqtt, "wifi2mqtt/esp8266_board_1");


// Application object
App app1 = App();


int getTempIndex(const char * id){
    for(int i = 0; i < numberOfDevices; i++){
        if (strcmp(getAddressToString(devAddr[i]).c_str(), id) == 0){
            return i;
        }
    }
    return 0;
}




void myTone(int freq, int duration)
{
    tone(BUZZER_PIN, freq, duration);
    delay(duration);
}


void getTempStr(char *buffer, const char * id){
    if(numberOfDevices == 0){
        sprintf(buffer, "no devices");
        return;
    }
    int index = getTempIndex(id);
    sprintf(buffer, "%.1f", tempDev[index]);
}

void humidityLoop() {
    //delay(dht.getMinimumSamplingPeriod());

    float h = app1.dht->getHumidity();
    float temperature = app1.dht->getTemperature();

    if(app1.dht->getStatus() != 0){
        return;
    }
    theHumidity = h;
    float heatIndex = app1.dht->computeHeatIndex(temperature, theHumidity, false);

    Serial.print(app1.dht->getStatusString());
    Serial.print("\t humidity: ");
    Serial.print(theHumidity, 1);
    Serial.print("\t t: ");
    Serial.print(temperature, 1);
    Serial.print("\t HeatIndex: ");
    Serial.print(heatIndex, 1);
    Serial.print("\t Free heap size: ");
    Serial.println(system_get_free_heap_size(), 1);

    char jsonStr[100];
    sprintf(jsonStr, "{\"hum\":%.0f, \"dht_temp\":%.1f, \"heat_index\":%.1f}", theHumidity, temperature, heatIndex);
    app1.webSocket->broadcastTXT(jsonStr);
}


//Loop measuring the temperature
void tempLoop(long now) {

    // skip temperature measurement if melody is playing
    if(app1.rtttl->isPlaying()){
        return;
    }

    const int JSON_SIZE = 200;

    if ( now - lastTempMeasTime > tempMeasInterval ) { // Take a measurement at a fixed time (tempMeasInterval = 1000ms, 1s)
        StaticJsonBuffer<JSON_SIZE> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        JsonObject& temperatures = root.createNestedObject("temperatures");

        for (int i = 0; i < numberOfDevices; i++) {
            tempDev[i] = app1.tempService->DS18B20->getTempC( devAddr[i] ); // Measuring temperature in Celsius and save the measured value to the array
            temperatures[getAddressToString(devAddr[i])] = tempDev[i];
        }

        char jsonStr[JSON_SIZE];
        root.prettyPrintTo(jsonStr, JSON_SIZE);
        app1.webSocket->broadcastTXT(jsonStr);
        
        app1.tempService->DS18B20->setWaitForConversion(false); //No waiting for measurement
        app1.tempService->DS18B20->requestTemperatures(); //Initiate the temperature measurement
        lastTempMeasTime = millis();  //Remember the last time measurement

        humidityLoop();
    }
}


void displayLoop() {
    long now = millis();
    if ( now - lastDispTime > updateDisplayInterval ) {
        char tempStr[8];
        char temp2Str[8];
        char humStr[8];
        getTempStr(tempStr, TEMP_ID_MAIN);
        getTempStr(temp2Str, TEMP_ID_SCND);
        sprintf(humStr, "%.0f", theHumidity);

        app1.display->clear(); // clearing the display

        app1.display->setFont(ArialMT_Plain_16);
        // first line (yellow)
        app1.display->drawString(90, 0, String("") + humStr + "%");
        // second line
        app1.display->drawString(5, 20, String("t = ") + tempStr + " ºC");

        app1.display->setFont(ArialMT_Plain_10);
        app1.display->drawString(90, 15, String("") + temp2Str + " ºC");
        app1.display->drawString(5, 40, String("ip: ") + WiFi.localIP().toString());

        // blinking pixel
        if (millis() / 200 % 2)
            app1.display->setPixel(0, 0);
        app1.display->display();
        lastDispTime = millis();
    }
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (app1.mqtt->connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = app1.mqtt->connect()) != 0) { // connect will return 0 for connected
       Serial.println(app1.mqtt->connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       app1.mqtt->disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         return;
       }
  }
  Serial.println("MQTT Connected!");
}

void mqttLoop() {
    const int JSON_SIZE = 300;
    long now = millis();
    if ( (!lastMqttPublishTime && tempDev[0] > 0) || now - lastMqttPublishTime > mqttPublishInterval ) {

        char tempStr[8];
        char temp2Str[8];
        char humStr[8];
        getTempStr(tempStr, TEMP_ID_MAIN);
        getTempStr(temp2Str, TEMP_ID_SCND);
        sprintf(humStr, "%.0f", theHumidity);

        StaticJsonBuffer<JSON_SIZE> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        //JsonArray& others = root.createNestedArray("others");
        JsonObject& temperatures = root.createNestedObject("temperatures");


        temperatures["main"] = tempStr;
        temperatures["second"] = temp2Str;
        root["humidity"] = humStr;

        char jsonStr[JSON_SIZE];
        root.prettyPrintTo(jsonStr, JSON_SIZE);

        MQTT_connect();
        if (!app1.mqtt_publish->publish(jsonStr)) {
          Serial.println("MQTT publish failed!");
        } else {
          Serial.println("MQTT publish ok!");
        }
        
        lastMqttPublishTime = millis();
    }
}


void logWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len) {
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
}



void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len) { // When a WebSocket message is received
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
}


void onAPStarted(WiFiManager * manager){
    app1.display->clear();
    app1.display->setFont(ArialMT_Plain_10);
    app1.display->setTextAlignment(TEXT_ALIGN_LEFT);
    int top = 20;
    app1.display->drawString(0, top + 0, String("Please connect to Wi-Fi"));
    app1.display->drawString(0, top + 10, String("Network: E-STOVE"));
    app1.display->drawString(0, top + 20, String("Password: 12341234"));
    app1.display->drawString(0, top + 30, String("Then go to ip: 10.0.1.1"));
    app1.display->display();
}


bool HandleFileRead(String path) {                              // send the right file to the client (if it exists)
    Serial.println("handleFileRead: " + path);
    if (path.endsWith("/")) path += "index.html";               // If a folder is requested, send the index file
    String contentType = getContentType(path);                  // Get the MIME type
    String pathWithGz = path + ".gz";
    if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {     // If the file exists, either as a compressed archive, or normal
        Serial.println(String("\tFile exists: ") + path);
        if (SPIFFS.exists(pathWithGz))                          // If there's a compressed version available
            path += ".gz";                                      // Use the compressed verion
        File file = SPIFFS.open(path, "r");                     // Open the file
        size_t sent = app1.server->streamFile(file, contentType);     // Send it to the client
        file.close();                                           // Close the file again
        Serial.println(String("\tSent file: ") + path);
        return true;
    }
    Serial.println(String("\tFile Not Found: ") + path);        // If the file doesn't exist, return false
    return false;
}

void HandleNotFound(){
    if(!HandleFileRead(app1.server->uri())){                          // check if the file exists in the flash memory (SPIFFS), if so, send it
        String message = "File Not Found\n\n";
        message += "URI: ";
        message += app1.server->uri();
        message += "\nMethod: ";
        message += (app1.server->method() == HTTP_GET)?"GET":"POST";
        message += "\nArguments: ";
        message += app1.server->args();
        message += "\n";
        for (uint8_t i=0; i<app1.server->args(); i++){
            message += " " + app1.server->argName(i) + ": " + app1.server->arg(i) + "\n";
        }
        app1.server->send(404, "text/html", message);
    }
}



//Setting the temperature sensor
void SetupDS18B20() {
    app1.tempService->DS18B20->begin();

    numberOfDevices = app1.tempService->DS18B20->getDeviceCount();

    // Loop through each device, print out address
    for (int i = 0; i < numberOfDevices; i++) {
        // save device's address
        app1.tempService->DS18B20->getAddress(devAddr[i], i);
    }

}

//------------------------------------------
void setup() {

    // Play first start melody
    myTone(800, 100);
    myTone(400, 100);
    myTone(1200, 100);

    app1.init();

    // Setup Serial port speed
    Serial.begin(115200);

    // Setup FileSystem
    SPIFFS.begin();


    // Setup display
    app1.display->init();
    app1.display->flipScreenVertically();
    app1.display->clear();
    app1.display->setFont(ArialMT_Plain_16);


    // Setup WIFI
    app1.display->drawString(20,20, "Hello Oleg!");
    app1.display->setFont(ArialMT_Plain_10);
    app1.display->drawString(20,50, "connecting to wifi..");
    app1.display->display();
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
    app1.server->on("/data", [](){
        String message = "";
        char temperatureString[8];
        for(int i=0; i<numberOfDevices; i++){
            dtostrf(tempDev[i], 2, 1, temperatureString);
            message += temperatureString;
            message += "\n";
        }

        app1.server->send(200, "text/html", message);        
    });
    app1.server->on("/play", [](){
        String melody = app1.server->arg("melody");

        if(melody.length() > 0){
            app1.rtttl->play(melody);
            app1.server->send(200, "text/html", String("Playing melody: ") + melody);        
        }
        else
            app1.server->send(400, "text/html", "'melody' GET parameter is required");
    });
    app1.server->on("/logout", [](){
        if(app1.server->method() == HTTP_POST){
            app1.server->send(200, "text/html", "OK");
            app1.wifiManager->resetSettings();
        }
        else{
            app1.server->send(400, "text/html", "post method only");
        }
    });
    app1.server->onNotFound( HandleNotFound );
    app1.server->begin();
    Serial.println("HTTP server started at ip " + WiFi.localIP().toString() );


    // Setup Web Socket
    app1.webSocket->begin();                          // start the websocket server
    app1.webSocket->onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
    Serial.println("WebSocket server started.");

    // Setup Logging web socket
    app1.logSocket->begin();                          // start the websocket server (for logging)
    app1.logSocket->onEvent(logWebSocketEvent);


    // Setup DS18b20 temperature sensor
    SetupDS18B20();

    // Play second start melody
    myTone(800, 100);
    myTone(400, 100);
    myTone(1200, 100);

    myTone(800, 100);
    myTone(400, 100);
    myTone(1200, 100);


    Serial.println("*** end setup ***");
}




void loop() {
    long t = millis();

    app1.server->handleClient();
    app1.webSocket->loop();
    app1.logSocket->loop();
    tempLoop( t );
    displayLoop();
    mqttLoop();
    app1.rtttl->updateMelody();

    loopCount ++;
}