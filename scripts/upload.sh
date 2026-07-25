#!/bin/bash

if [[ $PWD == *"scripts"* ]]; then
    PROJECTDIR=$(dirname $PWD)
else
    PROJECTDIR=$PWD
fi

# Read a PlatformIO setting, preferring the git-ignored per-machine override
# (platformio.local.ini) and falling back to the tracked platformio.ini.
# See platformio.local.ini.example for how to create the local file.
CONFIG_FILES=("$PROJECTDIR/platformio.local.ini" "$PROJECTDIR/platformio.ini")
read_ini() { # $1 = key name, e.g. upload_port
    local key=$1 file value
    for file in "${CONFIG_FILES[@]}"; do
        [[ -f $file ]] || continue
        value=$(awk -F '=' -v k="$key" '$1 ~ "^"k"[ \t]*$" {gsub(/[ \t]/, "", $2); print $2; exit}' "$file")
        if [[ -n $value ]]; then
            echo "$value"
            return
        fi
    done
}

UPLOAD_PORT=$(read_ini upload_port)
UPLOAD_SPEED=$(read_ini upload_speed)

if [[ -z "$UPLOAD_PORT" ]]; then
    echo "upload_port not set. Copy platformio.local.ini.example to platformio.local.ini and set upload_port (find it with: pio device list)."
    exit 1
fi

if [[ -z "$UPLOAD_SPEED" ]]; then
    echo "upload_speed not set in platformio.local.ini or platformio.ini"
    exit 1
fi

if [[ $1 == "--fs" ]]; then
    FILES=""
else
    if [ ! -f $PROJECTDIR/.pio/build/ulanzi_debug/firmware.bin ]; then
        echo "Firmware not found. Please build firmware first."
        exit 1
    fi
    FILES="0x10000 $PROJECTDIR/.pio/build/ulanzi_debug/firmware.bin"
fi

if [[ $1 == "--fs" || $1 == "--all" ]]; then
    if [ ! -f $PROJECTDIR/.pio/build/ulanzi_debug/littlefs.bin ]; then
        echo "LittleFS not found. Please build LittleFS first."
        exit 1
    fi
    FILES="$FILES \
    0x210000 $PROJECTDIR/.pio/build/ulanzi_debug/littlefs.bin"
fi

if [[ $1 == "--all" ]]; then
    FILES="0x1000 $PROJECTDIR/.pio/build/ulanzi_debug/bootloader.bin \
           0x8000 $PROJECTDIR/.pio/build/ulanzi_debug/partitions.bin \
           0xe000 $(realpath ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin) \
           $FILES"
fi

while true; do
    python ~/.platformio/packages/tool-esptoolpy/esptool.py \
     --chip esp32 --port "$UPLOAD_PORT" --baud "$UPLOAD_SPEED" --before default_reset --after hard_reset \
     write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB \
     $FILES
    if [ $? -eq 0 ]; then
        break
    fi
    echo "Upload failed. Retrying..."
done

echo "Upload done"

sleep 2

if [[ $1 == "--monitor" || $2 == "--monitor" ]]; then
    echo "Starting serial monitor..."
    bash "$PROJECTDIR/scripts/monitor.sh"
# else
#     echo "Let's reset the device"
#     bash "$PROJECTDIR/scripts/reset.sh"
fi
