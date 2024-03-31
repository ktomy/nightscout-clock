#include <BGAlarmManager.h>
#include "globals.h"

// The getter for the instantiated singleton instance
BGAlarmManager_ &BGAlarmManager_::getInstance() {
    static BGAlarmManager_ instance;
    return instance;
}

// Initialize the global shared instance
BGAlarmManager_ &bgAlarmManager = bgAlarmManager.getInstance();

void BGAlarmManager_::setup() {}

void BGAlarmManager_::tick() {}