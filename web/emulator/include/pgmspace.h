// Emulator stand-in: flash and RAM are the same address space here, as on the ESP32.
#ifndef EMU_PGMSPACE_H
#define EMU_PGMSPACE_H

#include <stdint.h>
#include <string.h>

#define PROGMEM
#define PGM_P const char*
#define PGM_VOID_P const void*
#define PSTR(s) (s)
#define _SFR_BYTE(n) (n)

#define pgm_read_byte(addr) (*(const unsigned char*)(addr))
#define pgm_read_word(addr) (*(const unsigned short*)(addr))
#define pgm_read_dword(addr) (*(const unsigned long*)(addr))
#define pgm_read_float(addr) (*(const float*)(addr))
#define pgm_read_ptr(addr) (*(const void**)(addr))

#define memcmp_P memcmp
#define memccpy_P memccpy
#define memmem_P memmem
#define memcpy_P memcpy
#define strcpy_P strcpy
#define strncpy_P strncpy
#define strcat_P strcat
#define strncat_P strncat
#define strcmp_P strcmp
#define strncmp_P strncmp
#define strcasecmp_P strcasecmp
#define strncasecmp_P strncasecmp
#define strlen_P strlen
#define strnlen_P strnlen
#define strstr_P strstr
#define printf_P printf
#define sprintf_P sprintf
#define snprintf_P snprintf
#define vsnprintf_P vsnprintf

#endif
