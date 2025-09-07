#ifndef Enums_h

#define Enums_h

#include <Arduino.h>

enum class BG_UNIT : uint8_t {
    MGDL = 0,
    MMOLL = 1,
};

enum class BG_TREND : uint8_t {
    NONE = 0,
    DOUBLE_UP = 1,
    SINGLE_UP = 2,
    FORTY_FIVE_UP = 3,
    FLAT = 4,
    FORTY_FIVE_DOWN = 5,
    SINGLE_DOWN = 6,
    DOUBLE_DOWN = 7,
    NOT_COMPUTABLE = 8,
    RATE_OUT_OF_RANGE = 9,
};

enum class TEXT_ALIGNMENT : uint8_t {
    LEFT = 0,
    CENTER = 1,
    RIGHT = 2,
};

enum class FONT_TYPE : uint8_t {
    SMALL = 0,
    MEDIUM = 1,
    LARGE = 2,
};

enum class BG_LEVEL : uint8_t {
    INVALID = 0,
    URGENT_LOW = 1,
    WARNING_LOW = 2,
    NORMAL = 3,
    WARNING_HIGH = 4,
    URGENT_HIGH = 5,
};

enum class BG_SOURCE : uint8_t {
    NO_SOURCE = 0,
    NIGHTSCOUT = 1,
    DEXCOM = 2,
    MEDTRONIC = 3,
    API = 4,
    LIBRELINKUP = 5,
};

enum class DEXCOM_SERVER : uint8_t {
    INVALID = 0,
    US = 1,
    NON_US = 2,
};

enum class TIME_FORMAT : uint8_t {
    INVALID = 0,
    HOURS_12 = 1,
    HOURS_24 = 2,
};

enum class BRIGHTNES_MODE : uint8_t {
    MANUAL = 0,
    AUTO_LINEAR = 100,
    AUTO_DIMMED = 101,
};

inline String toString(BG_TREND trend) {
    switch (trend) {
        case BG_TREND::NONE:
            return "NONE";
        case BG_TREND::DOUBLE_UP:
            return "DOUBLE_UP";
        case BG_TREND::SINGLE_UP:
            return "SINGLE_UP";
        case BG_TREND::FORTY_FIVE_UP:
            return "FORTY_FIVE_UP";
        case BG_TREND::FLAT:
            return "FLAT";
        case BG_TREND::FORTY_FIVE_DOWN:
            return "FORTY_FIVE_DOWN";
        case BG_TREND::SINGLE_DOWN:
            return "SINGLE_DOWN";
        case BG_TREND::DOUBLE_DOWN:
            return "DOUBLE_DOWN";
        case BG_TREND::NOT_COMPUTABLE:
            return "NOT_COMPUTABLE";
        case BG_TREND::RATE_OUT_OF_RANGE:
            return "RATE_OUT_OF_RANGE";
        default:
            return "unknown";
    }
}

inline String toString(BRIGHTNES_MODE mode) {
    switch (mode) {
        case BRIGHTNES_MODE::MANUAL:
            return "MANUAL";
        case BRIGHTNES_MODE::AUTO_LINEAR:
            return "AUTO_LINEAR";
        case BRIGHTNES_MODE::AUTO_DIMMED:
            return "AUTO_DIMMED";
        default:
            return "unknown";
    }
}

inline String toString(BG_UNIT unit) {
    switch (unit) {
        case BG_UNIT::MGDL:
            return "MGDL";
        case BG_UNIT::MMOLL:
            return "MMOLL";
        default:
            return "unknown";
    }
}

inline String toString(BG_LEVEL level) {
    switch (level) {
        case BG_LEVEL::INVALID:
            return "INVALID";
        case BG_LEVEL::URGENT_LOW:
            return "URGENT_LOW";
        case BG_LEVEL::WARNING_LOW:
            return "WARNING_LOW";
        case BG_LEVEL::NORMAL:
            return "NORMAL";
        case BG_LEVEL::WARNING_HIGH:
            return "WARNING_HIGH";
        case BG_LEVEL::URGENT_HIGH:
            return "URGENT_HIGH";
        default:
            return "unknown";
    }
}

inline String toString(BG_SOURCE source) {
    switch (source) {
        case BG_SOURCE::NO_SOURCE:
            return "NO_SOURCE";
        case BG_SOURCE::NIGHTSCOUT:
            return "NIGHTSCOUT";
        case BG_SOURCE::DEXCOM:
            return "DEXCOM";
        case BG_SOURCE::MEDTRONIC:
            return "MEDTRONIC";
        case BG_SOURCE::API:
            return "API";
        case BG_SOURCE::LIBRELINKUP:
            return "LIBRELINKUP";
        default:
            return "unknown";
    }
}

#endif