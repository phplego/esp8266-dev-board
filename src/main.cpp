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




//------------------------------------------
void setup() {

    // Play first start melody
    myTone(800, 100);
    myTone(400, 100);
    myTone(1200, 100);

    app1.init();

    myTone(600, 100);
    myTone(300, 100);


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
    
    // On Access Point started (not called if wifi is configured)
    app1.wifiManager->setAPCallback([](WiFiManager*){
        app1.dispService->display->clear();
        app1.dispService->display->setFont(ArialMT_Plain_10);
        app1.dispService->display->setTextAlignment(TEXT_ALIGN_LEFT);
        int top = 20;
        app1.dispService->display->drawString(0, top + 0, String("Please connect to Wi-Fi"));
        app1.dispService->display->drawString(0, top + 10, String("Network: E-STOVE"));
        app1.dispService->display->drawString(0, top + 20, String("Password: 12341234"));
        app1.dispService->display->drawString(0, top + 30, String("Then go to ip: 10.0.1.1"));
        app1.dispService->display->display();
    });

    app1.wifiManager->setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    app1.wifiManager->autoConnect("E-STOVE", "12341234"); // IMPORTANT! Blocks execution. Waits until connected

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