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



#define ONE_WIRE_MAX_DEV    15                              // The maximum number of devices
#define ONE_WIRE_BUS        D3                              // Pin to which is attached a temperature sensors DS18B20
#define MELODY_PIN          D2                              // Pin connected to a speaker
#define DISPLAY_SDA_PIN     D6                              // Display SDA pin
#define DISPLAY_SCL_PIN     D7                              // Display SCL pin


DubRtttl          rtttl(MELODY_PIN);                        // Melody player
WiFiManager       wifiManager;                              // WiFi Manager
OneWire           oneWire(ONE_WIRE_BUS);                    
DallasTemperature DS18B20(&oneWire);                        // Temperature sensor access object    
DHTesp            dht;                                      // DHT-11

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
// ALERT MELODIES AND TEMPERATURES
int alert_count = 3;
char alert_melodies[3][100] = {
    "SuperMario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g",
    "SuperMarioUnder:d=4,o=6,b=100:32c,32p,32c7,32p,32a5,32p,32a,32p,32a#5,32p,32a#",
    "MissionImp:d=16,o=6,b=95:32d,32d#,32d,32d#,32d,32d#,32d,32d#,32d,32d,32d#,32e,32f,32f#"
};
int alert_temps[] = {37, 39, 40};

//------------------------------------------
// WIFI
const char* ssid = "<SSID>";
const char* password = "<PASS>";



//------------------------------------------
// HTTP
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);
WebSocketsServer logSocket(82);


//------------------------------------------
// DISPLAY
SSD1306  display (0x3c, DISPLAY_SDA_PIN, DISPLAY_SCL_PIN);


//------------------------------------------
// MQTT

#define MQTT_HOST  "192.168.1.2"    // MQTT host (m21.cloudmqtt.com)
#define MQTT_PORT  11883            // MQTT port (18076)   
#define MQTT_USER  "mfkrdxtb"       // Ingored if brocker allows guest connection
#define MQTT_PASS  "jD-qPTjdtV34"   // Ingored if brocker allows guest connection

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS);

// Publish object
Adafruit_MQTT_Publish mqtt_publish = Adafruit_MQTT_Publish(&mqtt, "wifi2mqtt/esp8266_board_1");



int getTempIndex(const char * id){
    for(int i = 0; i < numberOfDevices; i++){
        if (strcmp(getAddressToString(devAddr[i]).c_str(), id) == 0){
            return i;
        }
    }
    return 0;
}



class MyLog {
    public:

    static void print(const char * str) {
        Serial.print(str);
        logSocket.broadcastTXT(str);
    }

    static void println(const char * str) {
        Serial.println(str);
        logSocket.broadcastTXT(String(str) + "\n");
    }
};

class TempAlert {
    public:

    static void loop() {
        if(numberOfDevices == 0) return;

        float tempC = tempDev[getTempIndex(TEMP_ID_MAIN)];

        int prevWarnLevel = warnLevel;

        warnLevel = -1;
        for(int ii = 0; ii < alert_count; ii++){
            if(tempC >= alert_temps[ii]){
                warnLevel = ii;
            }
        }

        if(warnLevel != prevWarnLevel){
            if(warnLevel != -1){
                rtttl.play(alert_melodies[warnLevel]);
            }
        }
    }
};


void myTone(int freq, int duration)
{
    tone(MELODY_PIN, freq, duration);
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

    float h = dht.getHumidity();
    float temperature = dht.getTemperature();

    if(dht.getStatus() != 0){
        return;
    }
    theHumidity = h;
    float heatIndex = dht.computeHeatIndex(temperature, theHumidity, false);

    Serial.print(dht.getStatusString());
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
    webSocket.broadcastTXT(jsonStr);
}


//Loop measuring the temperature
void tempLoop(long now) {

    // skip temperature measurement if melody is playing
    if(rtttl.isPlaying()){
        return;
    }

    const int JSON_SIZE = 200;

    if ( now - lastTempMeasTime > tempMeasInterval ) { // Take a measurement at a fixed time (tempMeasInterval = 1000ms, 1s)
        StaticJsonBuffer<JSON_SIZE> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        JsonObject& temperatures = root.createNestedObject("temperatures");

        for (int i = 0; i < numberOfDevices; i++) {
            tempDev[i] = DS18B20.getTempC( devAddr[i] ); // Measuring temperature in Celsius and save the measured value to the array
            temperatures[getAddressToString(devAddr[i])] = tempDev[i];
        }

        char jsonStr[JSON_SIZE];
        root.prettyPrintTo(jsonStr, JSON_SIZE);
        webSocket.broadcastTXT(jsonStr);
        
        DS18B20.setWaitForConversion(false); //No waiting for measurement
        DS18B20.requestTemperatures(); //Initiate the temperature measurement
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

        display.clear(); // clearing the display

        display.setFont(ArialMT_Plain_16);
        // first line (yellow)
        display.drawString(90, 0, String("") + humStr + "%");
        // second line
        display.drawString(5, 20, String("t = ") + tempStr + " ºC");

        display.setFont(ArialMT_Plain_10);
        display.drawString(90, 15, String("") + temp2Str + " ºC");
        display.drawString(5, 40, String("ip: ") + WiFi.localIP().toString());

        // blinking pixel
        if (millis() / 200 % 2)
            display.setPixel(0, 0);
        display.display();
        lastDispTime = millis();
    }
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
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
        if (!mqtt_publish.publish(jsonStr)) {
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
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        }
            break;
        case WStype_TEXT:                     // if new text data is received
            Serial.printf("[%u] get Text: %s\n", num, payload);
            char str[256];
            sprintf(str, "[%u] get Text: %s\n", num, payload);
            logSocket.sendTXT(num, str);
            break;
    }
}



void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len) { // When a WebSocket message is received
    switch (type) {
        case WStype_DISCONNECTED:             // if the websocket is disconnected
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {              // if a new websocket connection is established
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        }
            break;
        case WStype_TEXT:                     // if new text data is received
            Serial.printf("[%u] get Text: %s\n", num, payload);
            if (payload[0] == '#') {            // we get RGB data
                // example
            }
            webSocket.sendTXT(num, payload, len);
            break;
    }
}


void onAPStarted(WiFiManager * manager){
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    int top = 20;
    display.drawString(0, top + 0, String("Please connect to Wi-Fi"));
    display.drawString(0, top + 10, String("Network: E-STOVE"));
    display.drawString(0, top + 20, String("Password: 12341234"));
    display.drawString(0, top + 30, String("Then go to ip: 10.0.1.1"));
    display.display();
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
        size_t sent = server.streamFile(file, contentType);     // Send it to the client
        file.close();                                           // Close the file again
        Serial.println(String("\tSent file: ") + path);
        return true;
    }
    Serial.println(String("\tFile Not Found: ") + path);        // If the file doesn't exist, return false
    return false;
}

void HandleNotFound(){
    if(!HandleFileRead(server.uri())){                          // check if the file exists in the flash memory (SPIFFS), if so, send it
        String message = "File Not Found\n\n";
        message += "URI: ";
        message += server.uri();
        message += "\nMethod: ";
        message += (server.method() == HTTP_GET)?"GET":"POST";
        message += "\nArguments: ";
        message += server.args();
        message += "\n";
        for (uint8_t i=0; i<server.args(); i++){
            message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
        }
        server.send(404, "text/html", message);
    }
}



//Setting the temperature sensor
void SetupDS18B20() {
    DS18B20.begin();

    Serial.print("Parasite power is: ");
    if ( DS18B20.isParasitePowerMode() ) {
        Serial.println("ON");
    } else {
        Serial.println("OFF");
    }

    numberOfDevices = DS18B20.getDeviceCount();
    Serial.print( "Device count: " );
    Serial.println( numberOfDevices );

    lastTempMeasTime = millis();
    DS18B20.requestTemperatures();

    // Loop through each device, print out address
    for (int i = 0; i < numberOfDevices; i++) {
        // Search the wire for address
        if ( DS18B20.getAddress(devAddr[i], i) ) {
            //devAddr[i] = tempDeviceAddress;
            Serial.print("Found device ");
            Serial.print(i, DEC);
            Serial.print(" with address: " + getAddressToString(devAddr[i]));
            Serial.println();
        } else {
            Serial.print("Found ghost device at ");
            Serial.print(i, DEC);
            Serial.print(" but could not detect address. Check power and cabling");
        }

        //Get resolution of DS18b20
        Serial.print("Resolution: ");
        Serial.print(DS18B20.getResolution( devAddr[i] ));
        Serial.println();

        //Read temperature from DS18b20
        float tempC = DS18B20.getTempC( devAddr[i] );
        Serial.print("Temp C: ");
        Serial.println(tempC);
    }

}

//------------------------------------------
void setup() {

    // Play first start melody
    myTone(800, 100);
    myTone(400, 100);
    myTone(1200, 100);

    // Setup Serial port speed
    Serial.begin(115200);

    // Setup FileSystem
    SPIFFS.begin();


    // Setup display
    display.init();
    display.flipScreenVertically();
    display.clear();
    display.setFont(ArialMT_Plain_16);


    // Setup WIFI
    display.drawString(20,20, "Hello Oleg!");
    display.setFont(ArialMT_Plain_10);
    display.drawString(20,50, "connecting to wifi..");
    display.display();
    wifiManager.setAPCallback(onAPStarted);
    wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    wifiManager.autoConnect("E-STOVE", "12341234"); // Blocks execution. Waits until connected

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
    server.on("/data", [](){
        String message = "";
        char temperatureString[8];
        for(int i=0; i<numberOfDevices; i++){
            dtostrf(tempDev[i], 2, 1, temperatureString);
            message += temperatureString;
            message += "\n";
        }

        server.send(200, "text/html", message);        
    });
    server.on("/play", [](){
        String melody = server.arg("melody");

        if(melody.length() > 0){
            rtttl.play(melody);
            server.send(200, "text/html", String("Playing melody: ") + melody);        
        }
        else
            server.send(400, "text/html", "'melody' GET parameter is required");
    });
    server.on("/logout", [](){
        if(server.method() == HTTP_POST){
            server.send(200, "text/html", "OK");
            wifiManager.resetSettings();
        }
        else{
            server.send(400, "text/html", "post method only");
        }
    });
    server.onNotFound( HandleNotFound );
    server.begin();
    Serial.println("HTTP server started at ip " + WiFi.localIP().toString() );

    // Setup Web Socket
    webSocket.begin();                          // start the websocket server
    webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
    Serial.println("WebSocket server started.");

    // Setup Logging web socket
    logSocket.begin();                          // start the websocket server (for logging)
    logSocket.onEvent(logWebSocketEvent);


    // Setup DS18b20 temperature sensor
    SetupDS18B20();

    // Setup DHT-11 humidity sensor
    dht.setup(D4, DHTesp::DHT11); // Connect DHT sensor to GPIO 4

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

    server.handleClient();
    webSocket.loop();
    logSocket.loop();
    tempLoop( t );
    TempAlert::loop();
    displayLoop();
    mqttLoop();
    rtttl.updateMelody();

    loopCount ++;
}