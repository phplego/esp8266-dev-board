#include "App.h"



// Application object
App application = App();


void myTone(int freq, int duration)
{
    tone(BUZZER_PIN, freq, duration);
    delay(duration);
}


void setup() {

    // Play first start melody
    myTone(800, 100);
    myTone(400, 100);
    myTone(1200, 100);

    // Init the application
    application.init();

    // Play second start melody
    myTone(1000, 100);
    myTone(500, 100);
    myTone(1500, 100);

    Serial.println("*** end setup ***");
}




void loop() 
{
    // Main application loop
    application.loop();
}