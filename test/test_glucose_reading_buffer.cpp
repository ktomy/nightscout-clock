// Tests the fixed-size buffer that keeps the glucose readings in epoch order.

#include <cassert>
#include <cstdio>
#include <list>
#include <vector>

#include "GlucoseReadingBuffer.h"

struct Reading {
    int sgv;
    unsigned long long epoch;
};

using Buffer = GlucoseReadingBuffer<Reading, 4>;

static std::vector<unsigned long long> epochs(const Buffer& buffer) {
    std::vector<unsigned long long> result;
    for (const Reading& reading : buffer) {
        result.push_back(reading.epoch);
    }
    return result;
}

static void keepsEpochOrder() {
    Buffer buffer;
    assert(buffer.empty());
    assert(buffer.add({100, 60}));
    assert(buffer.add({110, 180}));
    assert(buffer.add({105, 120}));
    assert(buffer.add({90, 0}));
    assert((epochs(buffer) == std::vector<unsigned long long>{0, 60, 120, 180}));
    assert(buffer.front().sgv == 90);
    assert(buffer.back().sgv == 110);
    assert(buffer.rbegin()->sgv == 110);
    assert(std::next(buffer.rbegin(), 3) + 1 == buffer.rend());
}

static void leavesOutReadingsItHas() {
    Buffer buffer;
    assert(buffer.add({100, 60}));
    assert(!buffer.add({101, 60}));
    assert(buffer.size() == 1);
    assert(buffer.back().sgv == 100);
}

static void dropsTheOldestWhenFull() {
    Buffer buffer;
    for (unsigned long long epoch : {60, 120, 180, 240}) {
        assert(buffer.add({100, epoch}));
    }
    assert(buffer.add({100, 300}));
    assert((epochs(buffer) == std::vector<unsigned long long>{120, 180, 240, 300}));
    // Between stored readings: the oldest still makes room.
    assert(buffer.add({100, 150}));
    assert((epochs(buffer) == std::vector<unsigned long long>{150, 180, 240, 300}));
    // Older than all of them, or already there: nothing changes.
    assert(!buffer.add({100, 60}));
    assert(!buffer.add({100, 180}));
    assert((epochs(buffer) == std::vector<unsigned long long>{150, 180, 240, 300}));
}

static void addsContainersAndCountsWhatItAdded() {
    Buffer buffer;
    buffer.add({100, 120});
    std::list<Reading> retrieved{{100, 180}, {100, 120}, {100, 60}};
    assert(buffer.addAll(retrieved) == 2);
    assert((epochs(buffer) == std::vector<unsigned long long>{60, 120, 180}));
    assert(buffer.addAll(retrieved) == 0);
}

static void removesOldReadings() {
    Buffer buffer;
    for (unsigned long long epoch : {60, 120, 180}) {
        buffer.add({100, epoch});
    }
    assert(buffer.removeOlderThan(0) == 0);
    assert(buffer.removeOlderThan(120) == 1);
    assert((epochs(buffer) == std::vector<unsigned long long>{120, 180}));
    assert(buffer.removeOlderThan(1000) == 2);
    assert(buffer.empty());
    assert(buffer.begin() == buffer.end());
}

static void copiesByAssignment() {
    Buffer source;
    Buffer copy;
    copy.add({1, 1});
    copy.add({2, 2});
    copy.add({3, 3});
    source.add({100, 60});
    copy = source;
    assert((epochs(copy) == std::vector<unsigned long long>{60}));
    source.clear();
    copy = source;
    assert(copy.empty());
}

int main() {
    keepsEpochOrder();
    leavesOutReadingsItHas();
    dropsTheOldestWhenFull();
    addsContainersAndCountsWhatItAdded();
    removesOldReadings();
    copiesByAssignment();
    puts("Glucose reading buffer tests passed");
    return 0;
}
