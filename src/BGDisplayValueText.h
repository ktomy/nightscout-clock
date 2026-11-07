#ifndef BGDisplayValueText_h
#define BGDisplayValueText_h

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

#include "GlucoseUnits.h"

// This header stays free of Arduino dependencies so that test/ can compile it on the host.
//
// Values and deltas are calculated the way Nightscout's main view does it, described in
// docs/NIGHTSCOUT_MMOL_AND_DELTA.md. Rounding is floor(x + 0.5), JavaScript's Math.round: std::round
// would turn -2.5 into -3 where Nightscout shows -2. All maths uses double, as JavaScript does.

// A mg/dL value in mmol/L, rounded to one decimal place.
inline double toMmol(double mgdl) { return std::floor(mgdl / MGDL_PER_MMOL * 10 + 0.5) / 10; }

// A reading's sgv is always raw mg/dL. This value is local to display code: whole mg/dL, or tenths
// of a mmol/L, so readings can be compared the way they are shown. For example, 193 is 107 in mmol/L.
inline int toDisplayTenths(int sgv, bool mmol) {
    if (!mmol) {
        return sgv;
    }
    return (int)std::floor(sgv / MGDL_PER_MMOL * 10 + 0.5);
}

// The on-screen text of a toDisplayTenths() value.
inline std::string formatDisplayTenths(int tenths, bool mmol) {
    if (!mmol) {
        return std::to_string(tenths);
    }
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d.%d", tenths / 10, tenths % 10);
    return buffer;
}

inline std::string formatReading(int sgv, bool mmol) {
    return formatDisplayTenths(toDisplayTenths(sgv, mmol), mmol);
}

// Surprise mode celebrates a reading shown as exactly 100 mg/dL or 5.6 mmol/L.
inline bool isSurpriseReading(int sgv, bool mmol) {
    return toDisplayTenths(sgv, mmol) == (mmol ? 56 : 100);
}

// Readings are grouped into buckets this wide, centred on the newest reading and every 5 minutes before.
constexpr int64_t DELTA_BUCKET_SECONDS = 5 * 60;
constexpr int DELTA_BUCKET_COUNT = 4;
// A reading at or below this is an error code or LOW, not a value to compare.
constexpr int DELTA_MIN_VALID_SGV = 39;
// A change across a longer gap is scaled to 5 minutes.
constexpr double DELTA_MAX_UNSCALED_MINUTES = 9;

struct DeltaResult {
    bool valid;
    double mgdl;
    double mmol;
    bool interpolated;
};

// Nightscout's delta for readings at epoch seconds, ignoring readings later than nowEpoch. mgdl is a
// whole number and mmol has one decimal place.
template <typename Readings>
DeltaResult calculateDelta(const Readings& readings, int64_t nowEpoch) {
    const DeltaResult none = {false, 0, 0, false};

    bool hasAnchor = false;
    int64_t anchor = 0;
    for (const auto& reading : readings) {
        const int64_t epoch = (int64_t)reading.epoch;
        if (epoch <= nowEpoch && (!hasAnchor || epoch > anchor)) {
            anchor = epoch;
            hasAnchor = true;
        }
    }
    if (!hasAnchor) {
        return none;
    }

    double sums[DELTA_BUCKET_COUNT] = {};
    int counts[DELTA_BUCKET_COUNT] = {};
    int64_t times[DELTA_BUCKET_COUNT] = {};
    for (const auto& reading : readings) {
        const int64_t epoch = (int64_t)reading.epoch;
        if (epoch > nowEpoch || reading.sgv <= DELTA_MIN_VALID_SGV) {
            continue;
        }
        // Both ends of a bucket are inclusive, and a reading on a boundary goes in the newer bucket.
        for (int i = 0; i < DELTA_BUCKET_COUNT; i++) {
            const int64_t centre = anchor - i * DELTA_BUCKET_SECONDS;
            if (epoch >= centre - DELTA_BUCKET_SECONDS / 2 &&
                epoch <= centre + DELTA_BUCKET_SECONDS / 2) {
                if (counts[i] == 0 || epoch > times[i]) {
                    times[i] = epoch;
                }
                sums[i] += reading.sgv;
                counts[i]++;
                break;
            }
        }
    }

    if (counts[0] == 0) {
        return none;
    }
    int previous = 1;
    while (previous < DELTA_BUCKET_COUNT && counts[previous] == 0) {
        previous++;
    }
    if (previous == DELTA_BUCKET_COUNT) {
        return none;
    }

    // Keep the order of operations from Nightscout, so floating-point results match.
    const double recentMean = sums[0] / counts[0];
    const double previousMean = sums[previous] / counts[previous];
    const double absolute = recentMean - previousMean;
    const double elapsedMins = (times[0] - times[previous]) / 60.0;
    const bool interpolated = elapsedMins > DELTA_MAX_UNSCALED_MINUTES;
    const double mean5MinsAgo =
        interpolated ? recentMean - absolute / elapsedMins * 5 : recentMean - absolute;

    return {
        true,
        std::floor(recentMean - mean5MinsAgo + 0.5),
        std::floor((toMmol(recentMean) - toMmol(mean5MinsAgo)) * 10 + 0.5) / 10,
        interpolated,
    };
}

// A rounded delta with its sign: +5 and -12 in mg/dL, +0.2 and -1.0 in mmol/L. Zero is never -0.
inline std::string formatDelta(double delta, bool mmol) {
    char value[16];
    snprintf(value, sizeof(value), mmol ? "%.1f" : "%.0f", std::fabs(delta));
    return std::string(delta < 0 ? "-" : "+") + value;
}

// The delta text for readings at epoch seconds: "?" when Nightscout would show no delta, or when the
// delta needs three digits and does not fit on the screen, from 100 mg/dL or 10.0 mmol/L.
template <typename Readings>
std::string formatDiff(const Readings& readings, bool mmol, int64_t nowEpoch) {
    const DeltaResult delta = calculateDelta(readings, nowEpoch);
    if (!delta.valid) {
        return "?";
    }
    const double value = mmol ? delta.mmol : delta.mgdl;
    if (std::fabs(value) >= (mmol ? 10.0 : 100.0)) {
        return "?";
    }
    return formatDelta(value, mmol);
}

#endif
