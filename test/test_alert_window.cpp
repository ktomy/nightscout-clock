// Tests the firmware's alert window check, including windows that cross midnight.

#include <cassert>
#include <cstdio>
#include <vector>

#include "SettingsAlertWindow.h"

enum { SUN, MON, TUE, WED, THU, FRI, SAT };

static int at(int hour, int minute) { return hour * 60 + minute; }

static AlertWindow window(std::vector<int> days, int start, int end) {
    AlertWindow result;
    for (int day : days) {
        result.days[day] = true;
    }
    result.startMinutes = start;
    result.endMinutes = end;
    return result;
}

static void sameDayWindow() {
    const std::vector<AlertWindow> windows = {window({MON}, at(9, 0), at(17, 0))};

    assert(isInsideAlertWindow(windows, at(12, 0), MON));
    assert(isInsideAlertWindow(windows, at(9, 0), MON));    // the start is included
    assert(!isInsideAlertWindow(windows, at(17, 0), MON));  // the end is not
    assert(!isInsideAlertWindow(windows, at(8, 59), MON));
    assert(!isInsideAlertWindow(windows, at(12, 0), TUE));
}

static void windowCrossingMidnight() {
    const std::vector<AlertWindow> windows = {window({MON}, at(22, 0), at(6, 0))};

    assert(isInsideAlertWindow(windows, at(23, 0), MON));
    assert(isInsideAlertWindow(windows, at(5, 59), TUE));  // the morning belongs to Monday's window
    assert(!isInsideAlertWindow(windows, at(6, 0), TUE));
    assert(!isInsideAlertWindow(windows, at(23, 0), TUE));
    assert(!isInsideAlertWindow(windows, at(5, 59), MON));  // Sunday has no window
    assert(!isInsideAlertWindow(windows, at(12, 0), MON));
}

static void windowCrossingTheEndOfTheWeek() {
    const std::vector<AlertWindow> windows = {window({SAT}, at(22, 0), at(6, 0))};

    assert(isInsideAlertWindow(windows, at(23, 0), SAT));
    assert(isInsideAlertWindow(windows, at(1, 0), SUN));
    assert(!isInsideAlertWindow(windows, at(1, 0), MON));
}

static void severalWindows() {
    const std::vector<AlertWindow> windows = {
        window({MON}, at(9, 0), at(10, 0)), window({TUE}, at(20, 0), at(21, 0))};

    assert(isInsideAlertWindow(windows, at(9, 30), MON));
    assert(isInsideAlertWindow(windows, at(20, 30), TUE));
    assert(!isInsideAlertWindow(windows, at(20, 30), MON));
    assert(!isInsideAlertWindow({}, at(12, 0), MON));
}

int main() {
    sameDayWindow();
    windowCrossingMidnight();
    windowCrossingTheEndOfTheWeek();
    severalWindows();
    std::puts("test_alert_window: all assertions passed");
    return 0;
}
