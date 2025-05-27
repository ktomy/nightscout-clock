#include "BGSourceManager.h"

#include "ServerManager.h"

// Define the static getInstance method
BGSourceManager_& BGSourceManager_::getInstance() {
    static BGSourceManager_ instance;
    return instance;
}

// Define the constructor
BGSourceManager_::BGSourceManager_() {}

// Define the destructor
BGSourceManager_::~BGSourceManager_() {}

// Define the extern variable
BGSourceManager_& bgSourceManager = BGSourceManager_::getInstance();

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
        case BG_SOURCE::LIBRELINKUP:
            bgSource = new BGSourceLibreLinkUp();
            break;
        case BG_SOURCE::MEDTRONIC:
            bgSource = new BGSourceMedtronic();
            break;
        default:
            DEBUG_PRINTLN(
                "BGSourceManager_::setup: Unknown BG_SOURCE: " + toString(bgSourceType) + " (" +
                String((int)bgSourceType) + ")");
            DisplayManager.showFatalError(
                "Unknown data source: " + toString(bgSourceType) + " (" + String((int)bgSourceType) +
                ")");
    }
    bgSource->setup();
}

void BGSourceManager_::tick() { bgSource->tick(); }

bool BGSourceManager_::hasNewData(unsigned long long epochToCompare) {
    return bgSource->hasNewData(epochToCompare);
}

std::list<GlucoseReading> BGSourceManager_::getGlucoseData() { return bgSource->getGlucoseData(); }
