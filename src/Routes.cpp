#include "Routes.h"


Routes::Routes(WiFiManager* _wifiManager, ESP8266WebServer* _server, DubRtttl* _rtttl)
{
    this->wifiManager   = _wifiManager;
    this->server        = _server;
    this->rtttl         = _rtttl;
}

void Routes::init()
{
    this->server->on("/info", [this](){
        String str = "<pre>"; 
        str += String() + "           Uptime: " + (millis() / 1000) + " \n";
        str += String() + "      FullVersion: " + ESP.getFullVersion() + " \n";
        str += String() + "      ESP Chip ID: " + ESP.getChipId() + " \n";
        str += String() + "       CpuFreqMHz: " + ESP.getCpuFreqMHz() + " \n";
        str += String() + "              VCC: " + ESP.getVcc() + " \n";
        str += String() + "         FreeHeap: " + ESP.getFreeHeap() + " \n";
        str += String() + "       SketchSize: " + ESP.getSketchSize() + " \n";
        str += String() + "  FreeSketchSpace: " + ESP.getFreeSketchSpace() + " \n";
        str += String() + "    FlashChipSize: " + ESP.getFlashChipSize() + " \n";
        str += String() + "FlashChipRealSize: " + ESP.getFlashChipRealSize() + " \n";
        str += "</pre>";
        server->send(200, "text/html", str);     
    });

    // Test route
    this->server->on("/hello", [this](){
       server->send(200, "text/html", "hello hello OK");     
    });

    // Play melody 
    this->server->on("/play", [this](){
        String melody = this->server->arg("melody");

        if(melody.length() > 0){
            this->rtttl->play(melody);
            this->server->send(200, "text/html", String("Playing melody: ") + melody);        
        }
        else
            this->server->send(400, "text/html", "'melody' GET parameter is required");
    });

    // Logout
    this->server->on("/logout", [this](){
        if(this->server->method() == HTTP_POST){
            this->server->send(200, "text/html", "OK");
            this->wifiManager->resetSettings();
        }
        else{
            this->server->send(400, "text/html", "post method only");
        }
    });

    // Trying to read file (if route not found)
    this->server->onNotFound( [this](){
        // check if the file exists in the flash memory (SPIFFS), if so, send it
        if(!this->handleFileRead(this->server->uri())){                         
            // Otherwise send 404 error 
            String message = "File Not Found\n\n";
            message += "URI: ";
            message += this->server->uri();
            message += "\nMethod: ";
            message += (this->server->method() == HTTP_GET)?"GET":"POST";
            message += "\nArguments: ";
            message += this->server->args();
            message += "\n";
            for (uint8_t i=0; i < this->server->args(); i++){
                message += " " + this->server->argName(i) + ": " + this->server->arg(i) + "\n";
            }
            this->server->send(404, "text/html", message);
        }
    } );

    this->server->begin();
    Serial.println("HTTP server started at ip " + WiFi.localIP().toString() );

}

bool Routes::handleFileRead(String path)
{
    Serial.println("handleFileRead: " + path);
    if (path.endsWith("/")) path += "index.html";               // If a folder is requested, send the index file
    String contentType = getContentType(path);                  // Get the MIME type
    String pathWithGz = path + ".gz";
    if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {     // If the file exists, either as a compressed archive, or normal
        Serial.println(String("\tFile exists: ") + path);
        if (SPIFFS.exists(pathWithGz))                          // If there's a compressed version available
            path += ".gz";                                      // Use the compressed verion
        File file = SPIFFS.open(path, "r");                     // Open the file
        size_t sent = this->server->streamFile(file, contentType);     // Send it to the client
        file.close();                                           // Close the file again
        Serial.println(String("\tSent file: ") + path);
        return true;
    }
    Serial.println(String("\tFile Not Found: ") + path);        // If the file doesn't exist, return false
    return false;

}

