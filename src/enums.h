#ifndef Enums_h

#define Enums_h

#include <Arduino.h>

enum class BG_UNIT : uint8_t {
    MGDL = 0,
    MMOLL = 1,
};

enum BG_TREND : uint8_t {
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
};

#endif