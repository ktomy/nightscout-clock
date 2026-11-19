#ifndef LibreLinkUpTimestamp_h
#define LibreLinkUpTimestamp_h

// This header stays free of Arduino dependencies so that test/ can compile it on the host.

#include <cstdio>
#include <cstring>

// Reads LibreLinkUp's FactoryTimestamp, a UTC time such as "11/18/2026 11:42:49 PM".
//
// It used to go through mktime() after switching the TZ variable to UTC and back. newlib's setenv()
// overwrites a value in place only when the new one is no longer, and otherwise allocates a new
// buffer without freeing the old one. Switching back from "UTC0" to the longer local zone therefore
// leaked one allocation for every reading: once a minute, and about 140 times for each graph.
namespace LibreLinkUpTimestamp {

// Days from 1970-01-01 to a date in the proleptic Gregorian calendar (Howard Hinnant's
// days_from_civil).
inline long long daysFromCivil(int year, int month, int day) {
    year -= month <= 2;
    const long long era = (year >= 0 ? year : year - 399) / 400;
    const long long yearOfEra = year - era * 400;
    const long long dayOfYear = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
    const long long dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return era * 146097 + dayOfEra - 719468;
}

// Returns the Unix time of the timestamp, or 0 when it cannot be read. A time without AM or PM is
// read as 24-hour.
inline unsigned long long toEpoch(const char* text) {
    if (text == nullptr) {
        return 0;
    }
    int month, day, year, hour, minute, second;
    char half[3] = {0};
    int fields =
        sscanf(text, "%d/%d/%d %d:%d:%d %2s", &month, &day, &year, &hour, &minute, &second, half);
    if (fields < 6 || month < 1 || month > 12 || day < 1 || day > 31 || year < 1970 || minute < 0 ||
        minute > 59 || second < 0 || second > 60) {
        return 0;
    }
    if (fields == 7) {
        if (hour < 1 || hour > 12) {
            return 0;
        }
        hour %= 12;  // 12 AM is midnight and 12 PM is noon
        if (strcmp(half, "PM") == 0) {
            hour += 12;
        } else if (strcmp(half, "AM") != 0) {
            return 0;
        }
    } else if (hour < 0 || hour > 23) {
        return 0;
    }
    return (unsigned long long)(daysFromCivil(year, month, day) * 86400LL + hour * 3600LL +
                                minute * 60LL + second);
}

}  // namespace LibreLinkUpTimestamp

#endif
