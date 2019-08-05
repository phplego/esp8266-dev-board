#include "App.h"


App::App()
{
    wifiManager     = new WiFiManager();
    server          = new ESP8266WebServer(80);
    tempService     = new TemperatureService();
    humService      = new HumidityService();
    dispService     = new DisplayService();
    mqttService     = new MQTTService();
    wsService       = new WebSocketService();
    rtttl           = new DubRtttl(BUZZER_PIN);
    routes          = new Routes(wifiManager, server, rtttl);
    changesDetector = new ChangesDetector<5>();
}


void App::init()
{
    // Setup Serial port speed
    Serial.begin(115200);

    // Setup FileSystem
    SPIFFS.begin();

    // Setup Display Serivce
    dispService->init(tempService, humService);

    String deviceName = "espDevBrd";
    WiFi.hostname(deviceName);

    // On Access Point started (not called if wifi is configured)
    this->wifiManager->setAPCallback([](WiFiManager* mgr){
        DisplayService::instance->display->clear();
        DisplayService::instance->display->setFont(ArialMT_Plain_10);
        DisplayService::instance->display->setTextAlignment(TEXT_ALIGN_LEFT);
        int top = 20;
        DisplayService::instance->display->drawString(0, top + 0, String("Please connect to Wi-Fi"));
        DisplayService::instance->display->drawString(0, top + 10, String("Network: ") + mgr->getConfigPortalSSID());
        DisplayService::instance->display->drawString(0, top + 20, String("Password: 12341234"));
        DisplayService::instance->display->drawString(0, top + 30, String("Then go to ip: 10.0.1.1"));
        DisplayService::instance->display->display();
    });

    this->wifiManager->setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    this->wifiManager->autoConnect(deviceName.c_str(), "12341234"); // IMPORTANT! Blocks execution. Waits until connected

    // Wait for WIFI connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(10);
        Serial.print(".");
    }


    Serial.print("\nConnected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());


    // Setup Temperature Service
    tempService->init(ONE_WIRE_BUS, rtttl);

    // Setup Humidity Service
    humService->init(D4);


    // Setup MQTT Service
    mqttService->init();

    // Setup WebSockets Service
    wsService->init();

    // Setup web routes 
    routes->init();

    


    changesDetector->setGetValuesCallback([](float* buf){
        buf[0] = TemperatureService::instance->getTemperatureByAddress(TemperatureService::ADDRESS_MAIN);
        buf[1] = TemperatureService::instance->getTemperatureByAddress(TemperatureService::ADDRESS_SCND);
        //buf[2] = TemperatureService::instance->getTemperature(2);
        //buf[3] = HumidityService::instance->getHumidity();
    });

    changesDetector->setChangesDetectedCallback([](){
        MQTTService::instance->publishState();
    });


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
    changesDetector->loop();
}