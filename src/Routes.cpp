#include "Routes.h"


Routes::Routes(ESP8266WebServer* srv){
    this->server = srv;


    this->server->on("/hello", [this](){
       server->send(200, "text/html", "hello hello OK");     
    });

}