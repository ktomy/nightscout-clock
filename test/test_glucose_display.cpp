// Tests the on-screen glucose text and the Value and diff face's delta.

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <string>

#include "BGDisplayValueText.h"

struct Reading {
    int sgv;
    unsigned long long epoch;
};

static const unsigned long long NOW = 1000000;
static const unsigned long long MINUTE = 60;

// Readings oldest first, the given number of seconds apart, ending at NOW.
static std::list<Reading> readingsApart(unsigned long long seconds, std::initializer_list<int> sgvs) {
    std::list<Reading> readings;
    unsigned long long epoch = NOW - seconds * (sgvs.size() - 1);
    for (const int sgv : sgvs) {
        readings.push_back({sgv, epoch});
        epoch += seconds;
    }
    return readings;
}

static std::string mmolDiff(unsigned long long seconds, std::initializer_list<int> sgvs) {
    return formatDiff(readingsApart(seconds, sgvs), true);
}

static std::string mgdlDiff(unsigned long long seconds, std::initializer_list<int> sgvs) {
    return formatDiff(readingsApart(seconds, sgvs), false);
}

// Reads text like "10.7", "+0.1" or "-1.2" as whole tenths, the way a person reads the screen.
static int shownTenths(const std::string& text) {
    int whole = 0;
    int tenth = 0;
    int sign = text[0] == '-' ? -1 : 1;
    const char* digits = text[0] == '-' || text[0] == '+' ? text.c_str() + 1 : text.c_str();
    assert(sscanf(digits, "%d.%d", &whole, &tenth) == 2);
    assert(tenth >= 0 && tenth <= 9);
    return sign * (whole * 10 + tenth);
}

static void mgdlReadingIsShownAsIs() {
    assert(formatReading(39, false) == "39");
    assert(formatReading(100, false) == "100");
    assert(formatReading(401, false) == "401");
}

static void mmolReadingIsRoundedToOneDecimal() {
    assert(formatReading(39, true) == "2.2");
    assert(formatReading(90, true) == "5.0");
    assert(formatReading(116, true) == "6.4");
    assert(formatReading(117, true) == "6.5");
    assert(formatReading(118, true) == "6.6");
    assert(formatReading(179, true) == "9.9");
    assert(formatReading(180, true) == "10.0");
    assert(formatReading(192, true) == "10.7");
    assert(formatReading(193, true) == "10.7");
    assert(formatReading(400, true) == "22.2");
}

static void everyMmolReadingRoundsToTheNearestTenth() {
    for (int sgv = 1; sgv <= 999; sgv++) {
        // sgv / 18 mmol/L, rounded to the nearest tenth in integer arithmetic.
        const int tenths = (sgv * 20 + 18) / 36;
        char expected[16];
        snprintf(expected, sizeof(expected), "%d.%d", tenths / 10, tenths % 10);
        assert(formatReading(sgv, true) == expected);
    }
}

static void equalNumbersOnScreenHaveAZeroDelta() {
    assert(mmolDiff(5 * MINUTE, {193, 192}) == "+0.0");
    assert(mmolDiff(5 * MINUTE, {192, 193}) == "+0.0");
}

static void consecutiveScreensAddUp() {
    // 6.5 then 6.6 then 6.6: the second 6.6 must not repeat +0.1.
    assert(mmolDiff(5 * MINUTE, {117, 118}) == "+0.1");
    assert(mmolDiff(5 * MINUTE, {118, 119}) == "+0.0");
    assert(mmolDiff(5 * MINUTE, {119, 120}) == "+0.1");
}

static void deltaMatchesTheScreenForEveryPairOfReadings() {
    int checked = 0;
    for (int previous = 40; previous <= 400; previous++) {
        for (int current = 40; current <= 400; current++) {
            const std::string diff = mmolDiff(5 * MINUTE, {previous, current});
            if (std::abs(current - previous) > 99) {
                assert(diff == "?");
                continue;
            }
            const int shownGap =
                shownTenths(formatReading(current, true)) - shownTenths(formatReading(previous, true));
            assert(shownTenths(diff) == shownGap);
            assert(diff != "-0.0");
            assert((diff[0] == '-') == (shownGap < 0));
            assert((diff[0] == '+') == (shownGap >= 0));
            checked++;
        }
    }
    assert(checked > 60000);
}

static void largerGapsUseTheDisplayedValues() {
    // The true difference, 4.94, would round to +4.9, but the screen goes from 5.6 to 10.6.
    assert(mmolDiff(5 * MINUTE, {101, 190}) == "+5.0");
    assert(mmolDiff(5 * MINUTE, {190, 101}) == "-5.0");
}

static void mgdlDeltaIsTheRawDifference() {
    assert(mgdlDiff(5 * MINUTE, {100, 100}) == "+0");
    assert(mgdlDiff(5 * MINUTE, {100, 101}) == "+1");
    assert(mgdlDiff(5 * MINUTE, {101, 100}) == "-1");
    for (int previous = 40; previous <= 400; previous++) {
        for (int current = previous - 99; current <= previous + 99; current++) {
            const int gap = current - previous;
            const std::string expected = (gap >= 0 ? "+" : "") + std::to_string(gap);
            assert(mgdlDiff(5 * MINUTE, {previous, current}) == expected);
        }
    }
}

static void tooBigAJumpShowsNoDelta() {
    assert(mgdlDiff(5 * MINUTE, {100, 199}) == "+99");
    assert(mgdlDiff(5 * MINUTE, {100, 200}) == "?");
    assert(mgdlDiff(5 * MINUTE, {200, 100}) == "?");
    assert(mmolDiff(5 * MINUTE, {100, 199}) == "+5.5");
    assert(mmolDiff(5 * MINUTE, {100, 200}) == "?");
    assert(mmolDiff(5 * MINUTE, {200, 100}) == "?");
}

static void noPreviousReadingShowsNoDelta() {
    assert(formatDiff(std::list<Reading>{}, false) == "?");
    assert(formatDiff(std::list<Reading>{}, true) == "?");
    assert(mgdlDiff(5 * MINUTE, {100}) == "?");
    assert(mmolDiff(5 * MINUTE, {100}) == "?");
}

static void gapPastTwentyMinutesShowsNoDelta() {
    assert(mgdlDiff(20 * MINUTE, {100, 120}) == "+5");
    assert(mmolDiff(20 * MINUTE, {100, 120}) == "+0.3");
    assert(mgdlDiff(20 * MINUTE + 1, {100, 120}) == "?");
    assert(mmolDiff(20 * MINUTE + 1, {100, 120}) == "?");
    assert(mgdlDiff(180 * MINUTE, {100, 100}) == "?");

    // Only the newest previous reading counts, even with older ones behind it.
    const std::list<Reading> readings = {{100, NOW - 40 * MINUTE}, {100, NOW - 21 * MINUTE}, {110, NOW}};
    assert(formatDiff(readings, false) == "?");
}

static void comparesDirectlyInsideTheWindow() {
    assert(mgdlDiff(DIFF_WINDOW_SECONDS, {100, 110}) == "+10");
}

static void gapPastTheWindowEstimatesTheLastFiveMinutes() {
    // 100 mg/dL 11 minutes ago and 110 mg/dL 1 minute ago: 10 in 10 minutes is 5 in 5 minutes.
    assert(mgdlDiff(10 * MINUTE, {100, 110}) == "+5");
    assert(mgdlDiff(10 * MINUTE, {110, 100}) == "-5");
    assert(mgdlDiff(10 * MINUTE, {100, 100}) == "+0");
    assert(mgdlDiff(DIFF_WINDOW_SECONDS + 1, {100, 110}) == "+8");
    assert(mgdlDiff(15 * MINUTE, {100, 115}) == "+5");

    // Rounds to the nearest whole number, or the nearest tenth of a mmol/L.
    assert(mgdlDiff(10 * MINUTE, {100, 103}) == "+2");
    assert(mgdlDiff(10 * MINUTE, {103, 100}) == "-2");
    assert(mgdlDiff(10 * MINUTE, {100, 101}) == "+1");
    assert(mmolDiff(10 * MINUTE, {100, 110}) == "+0.3");
    assert(mmolDiff(10 * MINUTE, {110, 100}) == "-0.3");
    assert(mmolDiff(20 * MINUTE, {100, 101}) == "+0.0");
    assert(mmolDiff(20 * MINUTE, {101, 100}) == "+0.0");
}

static void estimateUsesTheNewestReadingBeforeTheGap() {
    const std::list<Reading> readings = {
        {300, NOW - 40 * MINUTE}, {100, NOW - 20 * MINUTE}, {110, NOW - 10 * MINUTE}, {120, NOW}};
    assert(formatDiff(readings, false) == "+5");
}

static void tooBigAnEstimateShowsNoDelta() {
    assert(mgdlDiff(10 * MINUTE, {100, 298}) == "+99");
    assert(mgdlDiff(10 * MINUTE, {100, 300}) == "?");
    assert(mgdlDiff(10 * MINUTE, {100, 302}) == "?");
    assert(mmolDiff(10 * MINUTE, {302, 100}) == "?");
}

static void sameValueWithinAMinuteIsOneReading() {
    // Alone they are a single reading, so there is nothing to compare.
    assert(mgdlDiff(30, {110, 110}) == "?");
    assert(mmolDiff(30, {110, 110}) == "?");

    // The duplicate is not the previous reading, so the estimate uses the older one.
    const std::list<Reading> afterGap = {{100, NOW - 10 * MINUTE - 30}, {110, NOW - 30}, {110, NOW}};
    assert(formatDiff(afterGap, false) == "+5");

    // Duplicates do not count towards the readings in the window.
    assert(mgdlDiff(5 * MINUTE, {100, 105}) == "+5");
    const std::list<Reading> inWindow = {{100, NOW - 5 * MINUTE}, {105, NOW - 20}, {105, NOW}};
    assert(formatDiff(inWindow, false) == "+5");
}

static void sameValueAMinuteApartIsTwoReadings() {
    assert(mgdlDiff(MINUTE, {110, 110}) == "+0");

    const std::list<Reading> afterGap = {{100, NOW - 11 * MINUTE}, {110, NOW - MINUTE}, {110, NOW}};
    assert(formatDiff(afterGap, false) == "+0");
}

static void differentValuesWithinAMinuteAreTwoReadings() { assert(mgdlDiff(30, {110, 112}) == "+2"); }

// Several uploaders to one Nightscout deliver each reading more than once (issue #25). The delta must
// never be a false zero from comparing a reading with its own copy.
static void uploadersCopyingAReadingKeepTheRealDelta() {
    for (const unsigned long long offset : {5, 30, 59, 60, 90, 120}) {
        const std::list<Reading> copyA = {
            {96, NOW - 600},
            {96, NOW - 600 + offset},
            {100, NOW - 300},
            {100, NOW - 300 + offset},
            {104, NOW}};
        assert(formatDiff(copyA, false) == "+4");

        std::list<Reading> copyB = copyA;
        copyB.push_back({104, NOW + offset});
        assert(formatDiff(copyB, false) == "+4");
    }

    const std::list<Reading> threeUploaders = {{96, NOW - 310}, {96, NOW - 305}, {96, NOW - 300},
                                               {100, NOW - 10}, {100, NOW - 5},  {100, NOW}};
    assert(formatDiff(threeUploaders, false) == "+4");
}

static void uploadersOnOneMinuteReadingsCompareTheLastTwoDistinct() {
    const std::list<Reading> readings = {{100, NOW - 185}, {100, NOW - 180}, {101, NOW - 125},
                                         {101, NOW - 120}, {102, NOW - 65},  {102, NOW - 60},
                                         {103, NOW - 5},   {103, NOW}};
    assert(formatDiff(readings, false) == "+1");
}

static void uploadersAcrossAMissedReadingEstimateTheDelta() {
    const std::list<Reading> readings = {{100, NOW - 605}, {100, NOW - 600}, {110, NOW - 5}, {110, NOW}};
    assert(formatDiff(readings, false) == "+5");
}

// One-minute readings can arrive slightly less than a minute apart. Dropping the equal ones must not
// take them below the count that selects comparing the last two.
static void duplicatesStillCountAsFrequentReadings() {
    const std::list<Reading> wobbling = {{102, NOW - 295}, {102, NOW - 236}, {100, NOW - 177},
                                         {100, NOW - 118}, {101, NOW - 59},  {101, NOW}};
    assert(formatDiff(wobbling, false) == "+1");

    const std::list<Reading> lastMoved = {{100, NOW - 295}, {100, NOW - 236}, {101, NOW - 177},
                                          {101, NOW - 118}, {101, NOW - 59},  {102, NOW}};
    assert(formatDiff(lastMoved, false) == "+1");
}

static void manyReadingsCompareTheLastTwo() {
    assert(mgdlDiff(MINUTE, {100, 130, 90, 120, 110, 105, 107}) == "+2");
    assert(mmolDiff(MINUTE, {100, 130, 90, 120, 110, 192, 193}) == "+0.0");

    // Six readings are enough to compare the last two; with five the noise guard still applies.
    assert(mgdlDiff(MINUTE, {100, 130, 90, 120, 105, 107}) == "+2");
    assert(mgdlDiff(MINUTE, {100, 130, 90, 120, 107}) == "?");
}

// With up to five readings in the window, the delta is measured against the lowest or highest of them
// rather than against the previous reading. This pins the current behavior of the noise guard.
static void fewReadingsCompareAgainstTheWindowExtreme() {
    assert(mgdlDiff(2 * MINUTE, {100, 102, 104}) == "+4");
    assert(mgdlDiff(2 * MINUTE, {104, 102, 100}) == "-4");
    assert(mgdlDiff(2 * MINUTE, {100, 104, 102}) == "?");
    assert(mgdlDiff(2 * MINUTE, {104, 100, 102}) == "?");

    // 116 shows as 6.4 and 117 as 6.5, so this screen repeats 6.5 with a +0.1 delta.
    assert(mmolDiff(2 * MINUTE, {116, 117, 117}) == "+0.1");
}

int main() {
    mgdlReadingIsShownAsIs();
    mmolReadingIsRoundedToOneDecimal();
    everyMmolReadingRoundsToTheNearestTenth();
    equalNumbersOnScreenHaveAZeroDelta();
    consecutiveScreensAddUp();
    deltaMatchesTheScreenForEveryPairOfReadings();
    largerGapsUseTheDisplayedValues();
    mgdlDeltaIsTheRawDifference();
    tooBigAJumpShowsNoDelta();
    noPreviousReadingShowsNoDelta();
    gapPastTwentyMinutesShowsNoDelta();
    comparesDirectlyInsideTheWindow();
    gapPastTheWindowEstimatesTheLastFiveMinutes();
    estimateUsesTheNewestReadingBeforeTheGap();
    tooBigAnEstimateShowsNoDelta();
    sameValueWithinAMinuteIsOneReading();
    sameValueAMinuteApartIsTwoReadings();
    differentValuesWithinAMinuteAreTwoReadings();
    uploadersCopyingAReadingKeepTheRealDelta();
    uploadersOnOneMinuteReadingsCompareTheLastTwoDistinct();
    uploadersAcrossAMissedReadingEstimateTheDelta();
    duplicatesStillCountAsFrequentReadings();
    manyReadingsCompareTheLastTwo();
    fewReadingsCompareAgainstTheWindowExtreme();
    printf("test_glucose_display: all passed\n");
    return 0;
}
