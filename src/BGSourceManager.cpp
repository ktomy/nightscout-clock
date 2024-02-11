#include "BGSourceManager.h"
#include "ServerManager.h"

// Define the static getInstance method
BGSourceManager_ &BGSourceManager_::getInstance() {
    static BGSourceManager_ instance;
    return instance;
}

// Define the constructor
BGSourceManager_::BGSourceManager_() {}

// Define the destructor
BGSourceManager_::~BGSourceManager_() {}

// Define the extern variable
BGSourceManager_ &bgSourceManager = BGSourceManager_::getInstance();

void BGSourceManager_::setup(BG_SOURCE bgSourceType) {
    switch (bgSourceType) {
        case BG_SOURCE::NIGHTSCOUT:
            bgSource = new BGSourceNightscout();
            break;
        case BG_SOURCE::DEXCOM:
            bgSource = new BGSourceDexcom();
            break;
        case BG_SOURCE::API:
            bgSource = new BGSourceApi();
            break;
        default:
            DEBUG_PRINTLN("BGSourceManager_::setup: Unknown BG_SOURCE: " + toString(bgSourceType) + " (" +
                          String((int)bgSourceType) + ")");
            DisplayManager.showFatalError("Unknown data source: " + toString(bgSourceType) + " (" + String((int)bgSourceType) +
                                          ")");
    }
    bgSource->setup();
}

// #ifdef DEBUG_BG_SOURCE

// unsigned long tickDebugMills = 0;

// #endif

void BGSourceManager_::tick() {

    //     // We poll the data once a minute trying to sync this with the last received data

    //     auto currentEpoch = ServerManager.getUtcEpoch();

    // #ifdef DEBUG_BG_SOURCE

    //     if (millis() - tickDebugMills > 10000) {
    //         tickDebugMills = millis();
    //         // debug print current method, current epoch and last poll epoch
    //         if (currentEpoch - lastPollEpoch < 61) {
    //             DEBUG_PRINTF("currentEpoch: %lu, lastPollEpoch: %llu, delta: %llu, not calling the source", currentEpoch,
    //                          lastPollEpoch, currentEpoch - lastPollEpoch);
    //         }
    //     }

    // #endif

    //     if (currentEpoch - lastPollEpoch < 61) {
    //         return;
    //     }

    // #ifdef DEBUG_BG_SOURCE

    //     DEBUG_PRINTLN("BGSourceManager_::tick: Calling bgSource->tick()");

    // #endif

    bgSource->tick();

    // #ifdef DEBUG_BG_SOURCE

    //     DEBUG_PRINTLN("BGSourceManager_::tick: bgSource->tick() called");

    // #endif

    //     auto readings = bgSource->getGlucoseData();
    //     auto lastReadingEpoch = readings.size() > 0 ? readings.back().epoch : 0;
    //     if (lastPollEpoch < lastReadingEpoch) {
    //         lastPollEpoch = lastReadingEpoch + 5;
    //     } else {
    //         lastPollEpoch = currentEpoch;
    //     }
}

bool BGSourceManager_::hasNewData(unsigned long long epochToCompare) { return bgSource->hasNewData(epochToCompare); }

std::list<GlucoseReading> BGSourceManager_::getGlucoseData() { return bgSource->getGlucoseData(); }
