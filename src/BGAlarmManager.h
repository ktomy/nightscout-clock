#ifndef BGAlarmManager_h
#define BGAlarmManager_h

#include <Arduino.h>
#include <EasyButton.h>

class BGAlarmManager_ {
  private:
    BGAlarmManager_() = default;

  public:
    static BGAlarmManager_ &getInstance();
    void setup();
    void tick();
};

extern BGAlarmManager_ &bgAlarmManager;
#endif
