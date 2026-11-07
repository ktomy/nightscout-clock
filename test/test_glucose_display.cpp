// Tests the on-screen glucose text and the Value and diff face's delta against Nightscout's behaviour,
// described in docs/NIGHTSCOUT_MMOL_AND_DELTA.md.

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <string>
#include <utility>

#include "BGDisplayValueText.h"

struct Reading {
    int sgv;
    unsigned long long epoch;
};

static const int64_t NOW = 1000000;

// Readings given as {minutes relative to NOW, mg/dL}, as in the doc's test cases.
static std::list<Reading> at(std::initializer_list<std::pair<double, int>> points) {
    std::list<Reading> readings;
    for (const auto& point : points) {
        readings.push_back({point.second, (unsigned long long)(NOW + (int64_t)(point.first * 60))});
    }
    return readings;
}

static std::string mgdl(std::initializer_list<std::pair<double, int>> points) {
    return formatDiff(at(points), false, NOW);
}

static std::string mmol(std::initializer_list<std::pair<double, int>> points) {
    return formatDiff(at(points), true, NOW);
}

// Reads text like "10.7", "+0.1", "+1" or "-1.2" as whole tenths, the way a person reads the screen.
static int shownTenths(const std::string& text) {
    const int sign = text[0] == '-' ? -1 : 1;
    const char* digits = text[0] == '-' || text[0] == '+' ? text.c_str() + 1 : text.c_str();
    int whole = 0;
    int tenth = 0;
    const int parsed = sscanf(digits, "%d.%d", &whole, &tenth);
    assert(parsed >= 1);
    assert(tenth >= 0 && tenth <= 9);
    return sign * (whole * 10 + tenth);
}

static void mgdlReadingIsShownAsIs() {
    assert(formatReading(39, false) == "39");
    assert(formatReading(100, false) == "100");
    assert(formatReading(401, false) == "401");
}

static void mmolReadingMatchesNightscout() {
    const std::pair<int, const char*> table[] = {
        {40, "2.2"},   {54, "3.0"},   {55, "3.1"},   {70, "3.9"},   {71, "3.9"},
        {99, "5.5"},   {100, "5.6"},  {101, "5.6"},  {108, "6.0"},  {109, "6.1"},
        {180, "10.0"}, {181, "10.0"}, {250, "13.9"}, {400, "22.2"},
    };
    for (const auto& row : table) {
        assert(formatReading(row.first, true) == row.second);
    }
}

static void everyMmolReadingRoundsToTheNearestTenth() {
    for (long long sgv = 1; sgv <= 999; sgv++) {
        // sgv / 18.01559 mmol/L, rounded half up to the nearest tenth in integer arithmetic.
        const long long tenths = (sgv * 2000000 + 1801559) / 3603118;
        char expected[16];
        snprintf(expected, sizeof(expected), "%lld.%lld", tenths / 10, tenths % 10);
        assert(formatReading((int)sgv, true) == expected);
    }
}

static void surpriseReadingMatchesWhatIsShown() {
    for (int sgv = 1; sgv <= 400; sgv++) {
        assert(isSurpriseReading(sgv, false) == (formatReading(sgv, false) == "100"));
        assert(isSurpriseReading(sgv, true) == (formatReading(sgv, true) == "5.6"));
    }
    assert(isSurpriseReading(100, true) && isSurpriseReading(101, true));
    assert(!isSurpriseReading(99, true) && !isSurpriseReading(102, true));
    assert(!isSurpriseReading(101, false));
}

// The doc's test cases, produced by Nightscout's own code.
static void matchesNightscoutTestCases() {
    // Rising and falling.
    assert(mgdl({{-5, 100}, {0, 105}}) == "+5" && mmol({{-5, 100}, {0, 105}}) == "+0.2");
    assert(mgdl({{-5, 105}, {0, 100}}) == "-5" && mmol({{-5, 105}, {0, 100}}) == "-0.2");
    assert(mgdl({{-5, 120}, {0, 120}}) == "+0" && mmol({{-5, 120}, {0, 120}}) == "+0.0");
    // Rounds to the same mmol/L, then crosses an mmol/L step.
    assert(mgdl({{-5, 100}, {0, 101}}) == "+1" && mmol({{-5, 100}, {0, 101}}) == "+0.0");
    assert(mgdl({{-5, 108}, {0, 109}}) == "+1" && mmol({{-5, 108}, {0, 109}}) == "+0.1");
    // Large drop.
    assert(mgdl({{-5, 200}, {0, 110}}) == "-90" && mmol({{-5, 200}, {0, 110}}) == "-5");
    // Gaps of 9 minutes or less are not scaled.
    assert(mgdl({{-8, 100}, {0, 108}}) == "+8" && mmol({{-8, 100}, {0, 108}}) == "+0.4");
    assert(mgdl({{-9, 100}, {0, 109}}) == "+9" && mmol({{-9, 100}, {0, 109}}) == "+0.5");
    // Longer gaps are scaled to 5 minutes.
    assert(mgdl({{-9.5, 100}, {0, 110}}) == "+5" && mmol({{-9.5, 100}, {0, 110}}) == "+0.3");
    assert(mgdl({{-10, 100}, {0, 105}}) == "+3" && mmol({{-10, 100}, {0, 105}}) == "+0.1");
    // Half rounds up, also when falling: -2.5 is -2, not -3.
    assert(mgdl({{-10, 105}, {0, 100}}) == "-2" && mmol({{-10, 105}, {0, 100}}) == "-0.1");
    assert(mgdl({{-10, 121}, {0, 120}}) == "+0" && mmol({{-10, 121}, {0, 120}}) == "+0.0");
    assert(mgdl({{-15, 100}, {0, 115}}) == "+5" && mmol({{-15, 100}, {0, 115}}) == "+0.3");
    // Readings every minute: bucket averages.
    const auto everyMinute = {std::pair<double, int>{-7, 93},
                              {-6, 94},
                              {-5, 95},
                              {-4, 96},
                              {-3, 97},
                              {-2, 98},
                              {-1, 99},
                              {0, 100}};
    assert(mgdl(everyMinute) == "+4" && mmol(everyMinute) == "+0.2");
    // Two readings in the recent bucket.
    assert(mgdl({{-5, 100}, {-2, 110}, {0, 120}}) == "+15");
    assert(mmol({{-5, 100}, {-2, 110}, {0, 120}}) == "+0.8");
    // A reading on a bucket boundary goes in the newer bucket.
    assert(mgdl({{-5, 100}, {-2.5, 110}, {0, 130}}) == "+20");
    // Invalid readings and 39 (LOW) are skipped.
    assert(
        mgdl({{-10, 100}, {-5, 30}, {0, 110}}) == "+5" &&
        mmol({{-10, 100}, {-5, 30}, {0, 110}}) == "+0.3");
    assert(
        mgdl({{-10, 100}, {-5, 39}, {0, 110}}) == "+5" &&
        mmol({{-10, 100}, {-5, 39}, {0, 110}}) == "+0.3");
    // A reading in the future is ignored.
    assert(
        mgdl({{-5, 100}, {0, 105}, {5, 200}}) == "+5" &&
        mmol({{-5, 100}, {0, 105}, {5, 200}}) == "+0.2");
}

static void noDeltaShowsAQuestionMark() {
    assert(formatDiff(std::list<Reading>{}, false, NOW) == "?");
    assert(formatDiff(std::list<Reading>{}, true, NOW) == "?");
    assert(mgdl({{0, 100}}) == "?" && mmol({{0, 100}}) == "?");
    // The previous reading is too old.
    assert(mgdl({{-20, 100}, {0, 120}}) == "?" && mmol({{-20, 100}, {0, 120}}) == "?");
    // The newest reading is invalid.
    assert(mgdl({{-5, 100}, {0, 30}}) == "?" && mmol({{-5, 100}, {0, 30}}) == "?");
    // Only future readings.
    assert(mgdl({{1, 100}, {6, 120}}) == "?");
}

static void oldestBucketEndsAtSeventeenAndAHalfMinutes() {
    // 15 in 17.5 minutes is 4.3 in 5 minutes.
    assert(mgdl({{-17.5, 100}, {0, 115}}) == "+4");
    assert(mgdl({{-17.6, 100}, {0, 115}}) == "?");
}

static void deltaDoesNotExpire() {
    // Buckets are measured from the newest reading, not from now.
    assert(mgdl({{-65, 100}, {-60, 105}}) == "+5");
}

// Several uploaders to one Nightscout deliver each reading more than once (issue #25). Copies land in
// the same bucket with the same value, so they never cause a false zero.
static void uploadersCopyingAReadingKeepTheRealDelta() {
    for (const int64_t offset : {5, 30, 59, 60, 90, 120}) {
        std::list<Reading> copies = {
            {96, (unsigned long long)(NOW - 600)},
            {96, (unsigned long long)(NOW - 600 + offset)},
            {100, (unsigned long long)(NOW - 300)},
            {100, (unsigned long long)(NOW - 300 + offset)},
            {104, (unsigned long long)NOW}};
        assert(formatDiff(copies, false, NOW) == "+4");
        assert(formatDiff(copies, false, NOW + offset) == "+4");
    }
}

static void mmolDeltaMatchesTheScreenForEveryPairOfReadings() {
    int checked = 0;
    for (int previous = 40; previous <= 400; previous++) {
        for (int current = 40; current <= 400; current++) {
            const std::string diff = mmol({{-5, previous}, {0, current}});
            const int shownGap =
                shownTenths(formatReading(current, true)) - shownTenths(formatReading(previous, true));
            if (std::abs(shownGap) >= 100) {
                assert(diff == "?");
                continue;
            }
            assert(shownTenths(diff) == shownGap);
            assert((diff[0] == '-') == (shownGap < 0));
            assert(diff == "+0.0" || diff.substr(diff.size() - 2) != ".0");
            checked++;
        }
    }
    assert(checked == 97273);
}

static void mgdlDeltaIsTheRawDifference() {
    for (int previous = 40; previous <= 400; previous++) {
        for (int current = 40; current <= 400; current++) {
            const int gap = current - previous;
            const std::string expected =
                std::abs(gap) >= 100 ? "?" : (gap >= 0 ? "+" : "") + std::to_string(gap);
            assert(mgdl({{-5, previous}, {0, current}}) == expected);
        }
    }
}

// Three digits do not fit on the screen, so those deltas show "?".
static void deltaTooWideForTheScreenShowsAQuestionMark() {
    assert(mgdl({{-5, 100}, {0, 199}}) == "+99");
    assert(mgdl({{-5, 199}, {0, 100}}) == "-99");
    assert(mgdl({{-5, 100}, {0, 200}}) == "?");
    assert(mgdl({{-5, 200}, {0, 100}}) == "?");

    // 5.6 to 15.5 is +9.9, and 5.6 to 15.6 is +10.0.
    assert(formatReading(100, true) == "5.6");
    assert(formatReading(279, true) == "15.5");
    assert(formatReading(281, true) == "15.6");
    assert(mmol({{-5, 100}, {0, 279}}) == "+9.9");
    assert(mmol({{-5, 279}, {0, 100}}) == "-9.9");
    assert(mmol({{-5, 100}, {0, 281}}) == "?");
    assert(mmol({{-5, 281}, {0, 100}}) == "?");

    // A jump of 100 mg/dL fits in mmol/L.
    assert(mmol({{-5, 100}, {0, 200}}) == "+5.5");
}

static void formatDeltaDropsTrailingZeroAndNegativeZero() {
    assert(formatDelta(-0.0, true) == "+0.0");
    assert(formatDelta(-0.0, false) == "+0");
    assert(formatDelta(0.0, true) == "+0.0");
    assert(formatDelta(0.0, false) == "+0");
    assert(formatDelta(1.0, true) == "+1");
    assert(formatDelta(-5.0, true) == "-5");
    assert(formatDelta(0.2, true) == "+0.2");
    assert(formatDelta(-9.9, true) == "-9.9");
    assert(formatDelta(10.0, true) == "+10");
    assert(formatDelta(-12, false) == "-12");
}

static void calculateDeltaReportsInterpolation() {
    assert(!calculateDelta(at({{-9, 100}, {0, 109}}), NOW).interpolated);
    assert(calculateDelta(at({{-9.5, 100}, {0, 110}}), NOW).interpolated);
    assert(!calculateDelta(at({{0, 100}}), NOW).valid);
}

int main() {
    mgdlReadingIsShownAsIs();
    mmolReadingMatchesNightscout();
    everyMmolReadingRoundsToTheNearestTenth();
    surpriseReadingMatchesWhatIsShown();
    matchesNightscoutTestCases();
    noDeltaShowsAQuestionMark();
    oldestBucketEndsAtSeventeenAndAHalfMinutes();
    deltaDoesNotExpire();
    uploadersCopyingAReadingKeepTheRealDelta();
    mmolDeltaMatchesTheScreenForEveryPairOfReadings();
    mgdlDeltaIsTheRawDifference();
    deltaTooWideForTheScreenShowsAQuestionMark();
    formatDeltaDropsTrailingZeroAndNegativeZero();
    calculateDeltaReportsInterpolation();
    printf("test_glucose_display: all passed\n");
    return 0;
}
