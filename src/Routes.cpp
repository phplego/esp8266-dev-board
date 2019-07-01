#include "Routes.h"




Routes::Routes(ESP8266WebServer* srv, DubRtttl* _rtttl)
{
    this->server    = srv;
    this->rtttl     = _rtttl;

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


    this->server->onNotFound( [this](){
        if(!this->handleFileRead(this->server->uri())){                          // check if the file exists in the flash memory (SPIFFS), if so, send it
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

