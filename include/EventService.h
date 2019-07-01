#include <Arduino.h>

class EventService{
    public:
        void subscribe(const char * event, std::function<void(const char *)> callback);
};