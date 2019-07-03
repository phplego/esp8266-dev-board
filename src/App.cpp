#include "App.h"


App::App()
{

}


void App::init()
{
    wifiManager = new WiFiManager();
    server      = new ESP8266WebServer(80);
    tempService = new TemperatureService();
    humService  = new HumidityService();
    dispService = new DisplayService();
    mqttService = new MQTTService();
    wsService   = new WebSocketService();
    rtttl       = new DubRtttl(BUZZER_PIN);
    routes      = new Routes(server, rtttl);

    // Setup Serial port speed
    Serial.begin(115200);

    // Setup FileSystem
    SPIFFS.begin();

    // Setup Temperature Service
    tempService->init(ONE_WIRE_BUS, rtttl);

    // Setup Humidity Service
    humService->init(D4);

    // Setup Display Serivce
    dispService->init(tempService, humService);

    // Setup MQTT Service
    mqttService->init();

    // Setup WebSockets Service
    wsService->init();


    // TEST EVENTS (remove then)
    EventService es;

    int aaa = 1;
    es.subscribe("E1", [aaa](const char * event){
        
    });
}

void App::loop()
{
    server->handleClient();
    tempService->loop();
    humService->loop();
    dispService->loop();
    mqttService->loop();
    wsService->loop();
    rtttl->updateMelody();
}