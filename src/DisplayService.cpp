#include "DisplayService.h"

DisplayService::DisplayService()
{

}

void DisplayService::init(TemperatureService* ts, HumidityService* hs)
{
    tempService = ts;
    humService  = hs;
    display     = new SSD1306(0x3c, DISPLAY_SDA_PIN, DISPLAY_SCL_PIN);

}

void DisplayService::loop()
{
    long now = millis();
    if ( now - lastUpdateTime > interval ) {
        char tempStr[8];
        char temp2Str[8];
        char humStr[8];
        sprintf(tempStr, "%.1f", this->tempService->getTemperatureByAddress(TemperatureService::ADDRESS_MAIN));
        sprintf(temp2Str, "%.1f", this->tempService->getTemperatureByAddress(TemperatureService::ADDRESS_SCND));
        sprintf(humStr, "%.0f", this->humService->getHumidity());

        this->display->clear(); // clearing the display

        this->display->setFont(ArialMT_Plain_16);
        // first line (yellow)
        this->display->drawString(90, 0, String("") + humStr + "%");
        // second line
        this->display->drawString(5, 20, String("t = ") + tempStr + " ºC");

        this->display->setFont(ArialMT_Plain_10);
        this->display->drawString(90, 15, String("") + temp2Str + " ºC");
        this->display->drawString(5, 40, String("ip: ") + WiFi.localIP().toString());

        // blinking pixel
        if (millis() / 200 % 2)
            this->display->setPixel(0, 0);
        this->display->display();
        lastUpdateTime = millis();
    }    
}