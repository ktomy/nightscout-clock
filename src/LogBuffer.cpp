#include "LogBuffer.h"

char LogBuffer::buffer[LogBuffer::CAPACITY];
size_t LogBuffer::writePos = 0;
size_t LogBuffer::used = 0;
SemaphoreHandle_t LogBuffer::mutex = nullptr;

SemaphoreHandle_t LogBuffer::getMutex() {
    if (mutex == nullptr) {
        mutex = xSemaphoreCreateMutex();
    }
    return mutex;
}

void LogBuffer::append(const char* message) {
    if (message == nullptr) {
        return;
    }
    SemaphoreHandle_t m = getMutex();
    if (m == nullptr) {
        return;
    }
    if (xSemaphoreTake(m, portMAX_DELAY) != pdTRUE) {
        return;
    }
    for (const char* p = message; *p != '\0'; ++p) {
        buffer[writePos] = *p;
        writePos = (writePos + 1) % CAPACITY;
        if (used < CAPACITY) {
            ++used;
        }
    }
    xSemaphoreGive(m);
}

void LogBuffer::append(const String& message) { append(message.c_str()); }

String LogBuffer::get() {
    SemaphoreHandle_t m = getMutex();
    if (m == nullptr) {
        return String();
    }
    if (xSemaphoreTake(m, portMAX_DELAY) != pdTRUE) {
        return String();
    }
    String result;
    result.reserve(used + 1);
    // When the buffer wrapped, the oldest byte is at writePos; otherwise at 0.
    size_t start = used < CAPACITY ? 0 : writePos;
    for (size_t i = 0; i < used; ++i) {
        result += buffer[(start + i) % CAPACITY];
    }
    xSemaphoreGive(m);
    return result;
}
