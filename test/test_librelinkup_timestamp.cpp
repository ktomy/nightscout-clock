// Tests reading LibreLinkUp's FactoryTimestamp, a UTC time, into Unix time.

#include <cassert>
#include <cstdio>

#include "LibreLinkUpTimestamp.h"

using LibreLinkUpTimestamp::toEpoch;

static void readsTwelveHourTimes() {
    // The same timestamp the clock logged with the epoch of its newest reading.
    assert(toEpoch("11/18/2026 11:42:49 PM") == 1795045369ULL);
    assert(toEpoch("12/31/1999 11:59:59 PM") == 946684799ULL);
    assert(toEpoch("2/29/2024 1:00:00 AM") == 1709168400ULL);
}

static void readsNoonAndMidnight() {
    assert(toEpoch("11/18/2026 12:04:48 PM") == 1795003488ULL);
    assert(toEpoch("11/19/2026 12:03:02 AM") == 1795046582ULL);
    assert(toEpoch("01/01/1970 12:00:00 AM") == 0ULL);
}

static void readsTwentyFourHourTimes() {
    assert(toEpoch("11/18/2026 23:42:49") == 1795045369ULL);
    assert(toEpoch("11/18/2026 00:00:00") == 1795045369ULL - (23 * 3600 + 42 * 60 + 49));
}

static void rejectsWhatItCannotRead() {
    assert(toEpoch(nullptr) == 0);
    assert(toEpoch("") == 0);
    assert(toEpoch("2026-11-18T23:42:49Z") == 0);
    assert(toEpoch("11/18/2026") == 0);
    assert(toEpoch("13/18/2026 11:42:49 PM") == 0);
    assert(toEpoch("11/18/2026 13:42:49 PM") == 0);
    assert(toEpoch("11/18/2026 0:42:49 AM") == 0);
    assert(toEpoch("11/18/2026 11:42:49 XM") == 0);
    assert(toEpoch("11/18/2026 24:00:00") == 0);
}

int main() {
    readsTwelveHourTimes();
    readsNoonAndMidnight();
    readsTwentyFourHourTimes();
    rejectsWhatItCannotRead();
    puts("LibreLinkUp timestamp tests passed");
    return 0;
}
