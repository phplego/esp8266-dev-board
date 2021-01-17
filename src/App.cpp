#include "App.h"

App* App::instance = NULL;


App::App()
{
    rtttl           = new DubRtttl(BUZZER_PIN);
    wifiManager     = new WiFiManager();
    server          = new ESP8266WebServer(80);
    tempService     = new TemperatureService();
    humService      = new HumidityService();
    dispService     = new DisplayService();
    mqttService     = new MQTTService();
    wsService       = new WebSocketService();
    routes          = new Routes(wifiManager, server, rtttl);
    changesDetector = new ChangesDetector<5>();

    App::instance = this;
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

    wifi_set_sleep_type(NONE_SLEEP_T); // prevent wifi sleep (stronger connection)

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
    this->wifiManager->setConfigPortalTimeout(60);
    this->wifiManager->autoConnect(deviceName.c_str(), "12341234"); // IMPORTANT! Blocks execution. Waits until connected

    // Restart if not connected
    if (WiFi.status() != WL_CONNECTED) 
    {
        ESP.restart();
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
        buf[2] = TemperatureService::instance->getTemperatureByAddress(TemperatureService::ADDRESS_PROBE);
        //buf[2] = TemperatureService::instance->getTemperature(2);
        //buf[3] = HumidityService::instance->getHumidity();
    });

    changesDetector->setChangesDetectedCallback([](){
        MQTTService::instance->publishState();
    });


    // Setup MQTT subscription for the 'set' topic.
    mqttService->mqtt->subscribe(mqttService->mqtt_subscribe);

    mqttService->mqtt_subscribe->setCallback([](char *str, uint16_t len){
        Serial.print(String("Got mqtt message len="));
        Serial.println(len);
        char buf [len + 1];
        buf[len] = 0;
        strncpy(buf, str, len);

        Serial.println(String("Got mqtt message: ") + buf);
        StaticJsonBuffer<10000> jsonBuffer;
        // StaticJsonBuffer allocates memory on the stack, it can be
        // replaced by DynamicJsonBuffer which allocates in the heap.

        // Root of the object tree.
        //
        // It's a reference to the JsonObject, the actual bytes are inside the
        // JsonBuffer with all the other nodes of the object tree.
        // Memory is freed when jsonBuffer goes out of scope.
        JsonObject& root = jsonBuffer.parseObject(buf);

        // Test if parsing succeeds.
        if (!root.success()) {
            Serial.println("parseObject() failed");
            return;
        }

        // Fetch values.
        //
        // Most of the time, you can rely on the implicit casts.
        // In other case, you can do root["time"].as<long>();

        if(root.containsKey("melody")){
            const char* melody = root["melody"];

            // Print values.
            Serial.print("Playing melody: ");
            Serial.println(melody);

            App::instance->rtttl->play(melody);
        }

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