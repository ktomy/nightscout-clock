#ifndef TLSMEMORY_H
#define TLSMEMORY_H

#include <Arduino.h>

// A TLS connection needs one block of about 17 KB for each of its two record buffers, plus several
// KB more during the handshake. When the heap is too fragmented for those blocks, every handshake
// fails with a memory allocation error although plenty of memory may still be free, and the heap
// does not recover by itself. This routes mbedTLS allocations through a wrapper that counts the
// ones that fail, so the clock can restart instead of failing every request.
//
// A dedicated region reserved at startup for TLS would avoid the fragmentation, but the 60 KB it
// needs leaves the rest of the firmware about 35 KB. Loading the web UI needs about 70 KB for a
// moment, and when the heap ran out the clock lost its WiFi traffic until WiFi reconnected.
class TlsMemory {
public:
    // Call once in setup(), before the first TLS connection.
    static void setup();

    // Call from the main loop. When an mbedTLS allocation failed, the heap has no block large enough
    // and only a restart gives memory back, so this restarts the clock. Not in the first minutes
    // after starting, where a restart would not help and could repeat.
    static void restartIfOutOfMemory();

#ifdef DEBUG_MEMORY
    // The number of failed mbedTLS allocations, for the memory debug lines.
    static String stats();
#endif
};

#endif  // TLSMEMORY_H
