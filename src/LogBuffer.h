#ifndef LOGBUFFER_H
#define LOGBUFFER_H

#include <Arduino.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// In-memory ring buffer holding the most recent debug log output, served by
// GET /api/logs. Sized at 16KB to keep the memory footprint modest. Appends
// are guarded by a FreeRTOS mutex because both the main-loop task and the
// async web server task may log.
class LogBuffer {
public:
    static void append(const char* message);
    static void append(const String& message);
    // Returns the buffered log, oldest first.
    static String get();

private:
    static constexpr size_t CAPACITY = 16 * 1024;
    static char buffer[CAPACITY];
    static size_t writePos;
    static size_t used;
    static SemaphoreHandle_t mutex;
    static SemaphoreHandle_t getMutex();
};

#endif  // LOGBUFFER_H
