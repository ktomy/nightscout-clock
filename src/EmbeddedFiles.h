#ifndef EMBEDDEDFILES_H
#define EMBEDDEDFILES_H

#include <stddef.h>
#include <stdint.h>

// Files built into the firmware by board_build.embed_files in platformio.ini. The linker names each
// after its path in the project, with every character other than a letter or digit replaced by "_":
// data/index.html.gz becomes _binary_data_index_html_gz_start and ..._end.
#define DECLARE_EMBEDDED_FILE(name, symbol)                               \
    extern const uint8_t name##_start[] asm("_binary_" #symbol "_start"); \
    extern const uint8_t name##_end[] asm("_binary_" #symbol "_end");

struct EmbeddedFile {
    const uint8_t* data;
    size_t size;
};

inline EmbeddedFile embeddedFile(const uint8_t* start, const uint8_t* end) {
    return EmbeddedFile{start, static_cast<size_t>(end - start)};
}

#endif
