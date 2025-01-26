#!/bin/bash

if [[ $PWD == *"scripts"* ]]; then
    PROJECTDIR=$(dirname $PWD)
else
    PROJECTDIR=$PWD
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
           0xe000 ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
           $FILES"
fi

while true; do
    python ~/.platformio/packages/tool-esptoolpy/esptool.py \
     --chip esp32 --port "/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0" --baud 921600 --before default_reset --after hard_reset \
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
    $PROJECTDIR/scripts/monitor.sh
# else
#     echo "Let's reset the device"
#     $PROJECTDIR/scripts/reset.sh
fi


