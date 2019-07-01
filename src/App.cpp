#include "App.h"


App::App()
{

}


void App::init()
{
    wifiManager = new WiFiManager();
    server      = new ESP8266WebServer(80);
    webSocket   = new WebSocketsServer(81);
    logSocket   = new WebSocketsServer(82);
    routes      = new Routes(server);
    oneWire     = new OneWire(D3);
    DS18B20     = new DallasTemperature(oneWire);
    dht         = new DHTesp();
    display     = new SSD1306(0x3c, DISPLAY_SDA_PIN, DISPLAY_SCL_PIN);
    rtttl       = new DubRtttl(BUZZER_PIN);


    // Create an ESP8266 WiFiClient class to connect to the MQTT server.
    WiFiClient* client = new WiFiClient();

    // Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
    mqtt            = new Adafruit_MQTT_Client(client, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS);
    mqtt_publish    = new Adafruit_MQTT_Publish(mqtt, "wifi2mqtt/esp8266_board_1");

    // Setup DHT-11 humidity sensor
    dht->setup(D4, DHTesp::DHT11); // Connect DHT sensor to GPIO 4 
}