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
    // oneWire     = new OneWire(D3);
    // DS18B20     = new DallasTemperature(oneWire);
    tempService = new TemperatureService();
    humService  = new HumidityService();
    dispService = new DisplayService();
    mqttService = new MQTTService();
    rtttl       = new DubRtttl(BUZZER_PIN);
    routes      = new Routes(server, rtttl);

    // Setup Temperature Service
    tempService->init(ONE_WIRE_BUS, rtttl);

    // Setup Humidity Service
    humService->init(D4);

    // Setup Display Serivce
    dispService->init(tempService, humService);

    // Setup MQTT Service
    mqttService->init();

    // // Create an ESP8266 WiFiClient class to connect to the MQTT server.
    // WiFiClient* client = new WiFiClient();

    // // Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
    // mqtt            = new Adafruit_MQTT_Client(client, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS);
    // mqtt_publish    = new Adafruit_MQTT_Publish(mqtt, "wifi2mqtt/esp8266_board_1");


    EventService es;

    int aaa = 1;
    es.subscribe("E1", [aaa](const char * event){
        
    });
}

void App::loop()
{
    server->handleClient();
    webSocket->loop();
    logSocket->loop();
    tempService->loop();
    humService->loop();
    dispService->loop();
    mqttService->loop();
    rtttl->updateMelody();
}