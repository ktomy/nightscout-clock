#ifndef Enums_h

#define Enums_h

#include <Arduino.h>

enum BG_UNIT : uint8_t
{
    MGDL = 0,
    MMOLL = 1,
};

enum BG_TREND : uint8_t
{
    NONE = 0,
    DoubleUp = 1,
    SingleUp = 2,
    FortyFiveUp = 3,
    Flat = 4,
    FortyFiveDown = 5,
    SingleDown = 6,
    DoubleDown = 7,
    NOT_COMPUTABLE = 8,
    RATE_OUT_OF_RANGE = 9,
};

enum BG_LEVEL : uint8_t
{
    INVALID = 0,
    URGENT_LOW = 1,
    WARNING_LOW = 2,
    NORMAL = 3,
    WARNING_HIGH = 4,
    URGENT_HIGH = 5,
};

#endif