// Emulator stand-in: the uptime face reads the time since start in microseconds.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
int64_t esp_timer_get_time(void);
#ifdef __cplusplus
}
#endif
