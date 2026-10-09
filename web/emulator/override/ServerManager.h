// Replaces the firmware's ServerManager.h: the display code only reads the clock's time from it.
#ifndef ServerManager_h
#define ServerManager_h

#include <Arduino.h>
#include <IPAddress.h>
#include <time.h>

class ServerManager_ {
public:
    static ServerManager_& getInstance();
    bool isConnected = true;
    bool isInAPMode = false;
    IPAddress myIP;
    unsigned long getUtcEpoch();
    tm getTimezonedTime();
    bool tryGetTimezonedTime(tm& timeinfo);
};

extern ServerManager_& ServerManager;

#endif
