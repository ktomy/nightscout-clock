#ifndef BGDisplayValueText_h
#define BGDisplayValueText_h

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// This header stays free of Arduino dependencies so that test/ can compile it on the host.

// A reading's sgv is always raw mg/dL. This value is local to display code: whole mg/dL, or tenths
// of a mmol/L, so readings can be compared the way they are shown. For example, 193 is 107 in mmol/L.
inline int toDisplayTenths(int sgv, bool mmol) {
    if (!mmol) {
        return sgv;
    }
    return std::round((float)sgv / 1.8);
}

// The on-screen text of a toDisplayTenths() value or of a difference between two of them.
inline std::string formatDisplayTenths(int tenths, bool mmol) {
    if (!mmol) {
        return std::to_string(tenths);
    }
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%.1f", (float)tenths / 10);
    return buffer;
}

inline std::string formatReading(int sgv, bool mmol) {
    return formatDisplayTenths(toDisplayTenths(sgv, mmol), mmol);
}

// Surprise mode celebrates a reading shown as exactly 100 mg/dL or 5.6 mmol/L.
inline bool isSurpriseReading(int sgv, bool mmol) {
    return toDisplayTenths(sgv, mmol) == (mmol ? 56 : 100);
}

// Older readings than this are not compared directly with the last reading.
constexpr unsigned long long DIFF_WINDOW_SECONDS = 6 * 60 + 30;
// A delta estimated across a longer gap is scaled to this period.
constexpr unsigned long long DIFF_PERIOD_SECONDS = 5 * 60;
// With a longer gap the previous reading says nothing about the current change.
constexpr unsigned long long DIFF_MAX_GAP_SECONDS = 20 * 60;
// Readings of the same value closer together than this count as one reading.
constexpr unsigned long long DUPLICATE_READING_SECONDS = 60;

struct DiffReading {
    int sgv;
    unsigned long long epoch;
};

// The readings a delta depends on, newest first and without duplicates: those inside the window,
// plus the first one older than it. A run of duplicates keeps its newest reading.
template <typename Readings>
std::vector<DiffReading> getDiffReadings(const Readings& readings) {
    std::vector<DiffReading> found;
    for (auto it = readings.rbegin(); it != readings.rend(); ++it) {
        if (!found.empty()) {
            const DiffReading& newer = found.back();
            if (it->sgv == newer.sgv && newer.epoch - it->epoch < DUPLICATE_READING_SECONDS) {
                continue;
            }
            if (found.front().epoch - newer.epoch > DIFF_WINDOW_SECONDS) {
                break;
            }
        }
        found.push_back({it->sgv, it->epoch});
    }
    return found;
}

// How many readings the window holds, duplicates included. This reflects how often data arrives.
template <typename Readings>
size_t countReadingsInWindow(const Readings& readings) {
    size_t count = 0;
    for (auto it = readings.rbegin(); it != readings.rend(); ++it) {
        if (readings.back().epoch - it->epoch > DIFF_WINDOW_SECONDS) {
            break;
        }
        count++;
    }
    return count;
}

inline std::string formatSignedDisplayTenths(int tenths, bool mmol) {
    return (tenths >= 0 ? "+" : "") + formatDisplayTenths(tenths, mmol);
}

// The delta text for readings sorted oldest first: "?" with no previous reading, a previous reading
// more than 20 minutes old, too big a jump or noisy data. The guards use raw mg/dL, so the display unit
// never changes them.
//
// The delta is normally the difference between the two displayed values, not the rounded raw
// difference. 193 and 192 mg/dL both show as 10.7, so their delta must be +0.0 and not -0.1. For larger
// gaps this can differ by 0.1 from the rounded true difference, which keeps it consistent with the
// screen.
//
// When the previous reading is older than the window, the delta is instead an estimate of the change
// over the last five minutes, assuming a steady change across the gap.
template <typename Readings>
std::string formatDiff(const Readings& readings, bool mmol) {
    const std::vector<DiffReading> found = getDiffReadings(readings);
    if (found.size() < 2) {
        return "?";
    }

    const int base = found[0].sgv;
    const unsigned long long gapSeconds = found[0].epoch - found[1].epoch;
    if (gapSeconds > DIFF_MAX_GAP_SECONDS) {
        return "?";
    }
    if (gapSeconds > DIFF_WINDOW_SECONDS) {
        const double estimate = (double)(base - found[1].sgv) * DIFF_PERIOD_SECONDS / gapSeconds;
        if (std::fabs(estimate) > 99) {
            return "?";
        }
        return formatSignedDisplayTenths(std::round(mmol ? estimate / 1.8 : estimate), mmol);
    }

    std::vector<int> foundSgvs;
    for (const DiffReading& reading : found) {
        if (found[0].epoch - reading.epoch <= DIFF_WINDOW_SECONDS) {
            foundSgvs.push_back(reading.sgv);
        }
    }

    // If we have a lot of readings, we are probably on Libre, so we just compare the last 2 readings.
    if (countReadingsInWindow(readings) > 5) {
        foundSgvs.resize(2);
    }

    int minSGV = 9999;
    int maxSGV = 0;
    for (const int sgv : foundSgvs) {
        if (sgv < minSGV) {
            minSGV = sgv;
        }
        if (sgv > maxSGV) {
            maxSGV = sgv;
        }
    }

    if (minSGV != base && maxSGV != base) {
        return "?";
    }

    const int other = minSGV == base ? maxSGV : minSGV;
    if (std::abs(base - other) > 99) {
        return "?";
    }

    return formatSignedDisplayTenths(toDisplayTenths(base, mmol) - toDisplayTenths(other, mmol), mmol);
}

#endif
