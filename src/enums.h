#ifndef Enums_h

#define Enums_h

enum BG_UNIT {
    MGDL = 0,
    MMOLL = 1,
};

enum BG_TREND {
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

#endif