#include "TlsMemory.h"

#include <esp_heap_caps.h>
#include <mbedtls/platform.h>

#include "globals.h"

static volatile uint32_t failedCount = 0;

static constexpr unsigned long NO_RESTART_AFTER_START_MS = 10UL * 60UL * 1000UL;

// What mbedTLS uses without the wrapper (CONFIG_MBEDTLS_INTERNAL_MEM_ALLOC).
static constexpr uint32_t CAPS = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;

#ifdef DEBUG_MEMORY
// The WiFi task's WPA supplicant uses mbedTLS too.
static portMUX_TYPE countLock = portMUX_INITIALIZER_UNLOCKED;
static size_t bytesInUse = 0;
static size_t peakBytes = 0;

static void countBlock(void* p, bool allocated) {
    size_t size = heap_caps_get_allocated_size(p);
    portENTER_CRITICAL(&countLock);
    if (allocated) {
        bytesInUse += size;
        if (bytesInUse > peakBytes) {
            peakBytes = bytesInUse;
        }
    } else {
        bytesInUse = size > bytesInUse ? 0 : bytesInUse - size;
    }
    portEXIT_CRITICAL(&countLock);
}
#endif

static void* tlsCalloc(size_t count, size_t size) {
    void* p = heap_caps_calloc(count, size, CAPS);
    if (p == nullptr) {
        if (count != 0 && size != 0) {
            failedCount++;
        }
        return p;
    }
#ifdef DEBUG_MEMORY
    countBlock(p, true);
#endif
    return p;
}

static void tlsFree(void* p) {
    if (p == nullptr) {
        return;
    }
#ifdef DEBUG_MEMORY
    countBlock(p, false);
#endif
    heap_caps_free(p);
}

void TlsMemory::setup() { mbedtls_platform_set_calloc_free(tlsCalloc, tlsFree); }

void TlsMemory::restartIfOutOfMemory() {
    if (failedCount == 0 || millis() < NO_RESTART_AFTER_START_MS) {
        return;
    }
    DEBUG_PRINTF(
        "TLS could not get memory (%u failed allocations, free heap %u, largest block %u), restarting",
        failedCount, heap_caps_get_free_size(MALLOC_CAP_8BIT),
        heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    delay(500);  // let the serial output finish
    ESP.restart();
}

#ifdef DEBUG_MEMORY
String TlsMemory::stats() {
    portENTER_CRITICAL(&countLock);
    size_t inUse = bytesInUse;
    size_t peak = peakBytes;
    peakBytes = bytesInUse;
    portEXIT_CRITICAL(&countLock);
    char buf[96];
    snprintf(
        buf, sizeof(buf), "TLS memory in use %u, peak %u, allocation failures %u", inUse, peak,
        failedCount);
    return String(buf);
}
#endif
