#include "WebSocketService.h"


WebSocketService* WebSocketService::instance = NULL;

WebSocketService::WebSocketService()
{
    instance = this;
}

void WebSocketService::init()
{
    webSocket   = new WebSocketsServer(81);
    logSocket   = new WebSocketsServer(82);

    webSocket->begin();                          // start the websocket server
    webSocket->onEvent([this](uint8_t num, WStype_t type, uint8_t * payload, size_t len){
        switch (type) {
            case WStype_DISCONNECTED:             // if the websocket is disconnected
                Serial.printf("[%u] Disconnected!\n", num);
                break;
            case WStype_CONNECTED: {              // if a new websocket connection is established
                IPAddress ip = webSocket->remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            }
                break;
            case WStype_TEXT:                     // if new text data is received
                Serial.printf("[%u] get Text: %s\n", num, payload);
                if (payload[0] == '#') {            // we get RGB data
                    // example
                }
                webSocket->sendTXT(num, payload, len);
                break;
        }
    });

    // Setup Logging web socket
    logSocket->begin();                          // start the websocket server (for logging)
    logSocket->onEvent([this](uint8_t num, WStype_t type, uint8_t * payload, size_t len){
        switch (type) {
            case WStype_DISCONNECTED:             // if the websocket is disconnected
                Serial.printf("[%u] Disconnected!\n", num);
                break;
            case WStype_CONNECTED: {              // if a new websocket connection is established
                IPAddress ip = logSocket->remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            }
                break;
            case WStype_TEXT:                     // if new text data is received
                Serial.printf("[%u] get Text: %s\n", num, payload);
                char str[256];
                sprintf(str, "[%u] get Text: %s\n", num, payload);
                logSocket->sendTXT(num, str);
                break;
        }        
    });


}


void WebSocketService::loop()
{
    webSocket->loop();
    logSocket->loop();
}
