#include "BGSourceManager.h"

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
    }
}

void BGSourceManager_::tick() { bgSource->tick(); }

bool BGSourceManager_::hasNewData(unsigned long long epochToCompare) { return bgSource->hasNewData(epochToCompare); }

std::list<GlucoseReading> BGSourceManager_::getGlucoseData() { return bgSource->getGlucoseData(); }
