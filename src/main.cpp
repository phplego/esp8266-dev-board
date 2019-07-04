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





    // Play second start melody

    myTone(1000, 100);
    myTone(500, 100);
    myTone(1500, 100);


    Serial.println("*** end setup ***");
}




void loop() 
{
    // Main application loop
    app1.loop();
}