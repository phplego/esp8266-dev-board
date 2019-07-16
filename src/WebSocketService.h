#pragma once

#include <WebSocketsServer.h>



class WebSocketService {
    public:
        // static instance
        static WebSocketService* instance;

        // Interval in milliseconds of the data sending
        int                 interval            = 1000;     // 10 seconds by default
        long                lastUpdateTime      = 0;

        // sockets
        WebSocketsServer*   webSocket;
        WebSocketsServer*   logSocket;        


    public:
        WebSocketService();
        void    init();
        void    loop();
};