// Included first in every file: the firmware's time(NULL) reads the preview's clock.
#ifndef EMU_PRELUDE_H
#define EMU_PRELUDE_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif
time_t emu_time(time_t* out);
#ifdef __cplusplus
}
#endif

#define time(t) emu_time(t)

#endif
