#!/usr/bin/env bash
# Builds data/clockemu.js.gz, the settings page's clock preview: the firmware's display code from src/
# and lib/ compiled to WebAssembly with Emscripten, plus the stand-ins in this folder for the hardware.
#
# Needs one PlatformIO build of the ulanzi env (for the libraries and the Arduino core), Node, and
# Emscripten EMSDK_VERSION: on PATH, or EMSDK=<emsdk folder>.
set -euo pipefail

EMSDK_VERSION=6.0.9

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
LIBS="${FW_LIBS:-$ROOT/.pio/libdeps/ulanzi}"
CORE="${PLATFORMIO_CORE_DIR:-$HOME/.platformio}/packages/framework-arduinoespressif32/cores/esp32"
OUT="$ROOT/.pio/emulator"

if ! command -v emcc >/dev/null && [ -n "${EMSDK:-}" ]; then
    # shellcheck disable=SC1091
    source "$EMSDK/emsdk_env.sh" >/dev/null 2>&1
fi
command -v emcc >/dev/null || { echo "emcc not found: install emsdk $EMSDK_VERSION and set EMSDK" >&2; exit 1; }
case "$(emcc --version)" in *" $EMSDK_VERSION "*) ;; *) echo "Emscripten $EMSDK_VERSION is required" >&2; exit 1 ;; esac
[ -d "$LIBS/ArduinoJson" ] || { echo "no libraries in $LIBS: build the ulanzi env once" >&2; exit 1; }
[ -f "$CORE/WString.cpp" ] || { echo "no Arduino core in $CORE: build the ulanzi env once" >&2; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/src" "$OUT/obj"

# The display code and the headers it includes, with this folder's replacements copied over the
# network-facing ones, and the Arduino core's String, Print and Stream.
cp "$ROOT"/src/*.h "$ROOT"/lib/Fonts/*.h "$ROOT"/lib/LightResistor/* "$ROOT"/lib/MelodyPlayer/* \
    "$ROOT"/src/BGDisplayFace*.cpp "$ROOT"/src/Settings*.cpp "$ROOT"/src/{DisplayManager,BGDisplayManager}.cpp \
    "$ROOT"/src/{globals,PeripheryManager,BGAlarmManager}.cpp "$HERE"/*.cpp "$OUT/src/"
cp "$HERE"/override/*.h "$OUT/src/"
for f in WString.h WString.cpp Print.h Print.cpp Printable.h Stream.h Stream.cpp stdlib_noniso.h stdlib_noniso.c; do
    cp "$CORE/$f" "$OUT/src/"
done

FLAGS=(-Oz -w -DARDUINO=10819 -DESP32 -DULANZI -include "$HERE/include/emu_prelude.h" -I"$HERE/include" -I"$OUT/src"
       -I"$LIBS/Adafruit GFX Library" -I"$LIBS/Framebuffer GFX" -I"$LIBS/FastLED NeoMatrix" -I"$LIBS/ArduinoJson/src")
OBJS=()
for src in "$OUT"/src/*.c "$OUT"/src/*.cpp "$LIBS/Adafruit GFX Library/Adafruit_GFX.cpp" \
    "$LIBS/Framebuffer GFX/Framebuffer_GFX.cpp" "$LIBS/FastLED NeoMatrix/FastLED_NeoMatrix.cpp"; do
    obj="$OUT/obj/$(basename "${src%.*}").o"
    case "$src" in
        *.c) emcc -std=gnu11 "${FLAGS[@]}" -c "$src" -o "$obj" ;;
        *) em++ -std=gnu++17 "${FLAGS[@]}" -c "$src" -o "$obj" ;;
    esac
    OBJS+=("$obj")
done

# emmalloc and Closure keep the download small.
em++ -Oz "${OBJS[@]}" -o "$OUT/clockemu.js" -sMODULARIZE=1 -sEXPORT_NAME=createClockEmu -sSINGLE_FILE=1 \
    -sENVIRONMENT=web -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=4194304 -sSTACK_SIZE=262144 -sMALLOC=emmalloc \
    -sEXPORTED_RUNTIME_METHODS=ccall,HEAPU8 --closure 1

# Gzipped like web/build.mjs does, so every platform writes the same bytes.
node -e 'const fs = require("fs"), zlib = require("zlib");
const out = zlib.gzipSync(fs.readFileSync(process.argv[1]), { level: 9 });
out[9] = 255;
fs.writeFileSync(process.argv[2], out);' "$OUT/clockemu.js" "$ROOT/data/clockemu.js.gz"
echo "data/clockemu.js.gz $(wc -c < "$ROOT/data/clockemu.js.gz" | tr -d ' ') B"
