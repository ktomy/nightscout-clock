#include "BGSourceManager.h"

// Define the static getInstance method
BGSourceManager_ &BGSourceManager_::getInstance()
{
    static BGSourceManager_ instance;
    return instance;
}

// Define the constructor
BGSourceManager_::BGSourceManager_() {}

// Define the destructor
BGSourceManager_::~BGSourceManager_() {}

// Define the extern variable
BGSourceManager_ &bgSourceManager = BGSourceManager_::getInstance();

void BGSourceManager_::setup()
{
    // TODO: Implement the setup method
}

void BGSourceManager_::tick()
{
    // TODO: Implement the tick method
}

bool BGSourceManager_::hasNewData(unsigned long long epochToCompare)
{
    // TODO: Implement the hasNewData method
    return false;
}

std::list<GlucoseReading> BGSourceManager_::getGlucoseData()
{
    // TODO: Implement the getGlucoseData method
    std::list<GlucoseReading> glucoseData;
    return glucoseData;
}
