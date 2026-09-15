#include "SettingsTime.h"

int parseTimeOfDayMinutes(const String& value) {
    int colon = value.indexOf(':');
    if (colon < 1 || (int)value.length() - colon != 3) {
        return -1;
    }
    for (unsigned int i = 0; i < value.length(); i++) {
        if ((int)i != colon && !isDigit(value[i])) {
            return -1;
        }
    }

    int hours = value.substring(0, colon).toInt();
    int minutes = value.substring(colon + 1).toInt();
    if (hours > 23 || minutes > 59) {
        return -1;
    }
    return hours * 60 + minutes;
}

String minutesAsTimeOfDay(int minutes) {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes / 60, minutes % 60);
    return String(buffer);
}
