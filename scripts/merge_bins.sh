#!/bin/bash

# Determine the project directory
if [[ $PWD == *"scripts"* ]]; then
    PROJECTDIR=$(dirname $PWD)
else
    PROJECTDIR=$PWD
fi

# Paths to binaries
BOOTLOADER="$PROJECTDIR/.pio/build/ulanzi_debug/bootloader.bin"
PARTITIONS="$PROJECTDIR/.pio/build/ulanzi_debug/partitions.bin"
FIRMWARE="$PROJECTDIR/.pio/build/ulanzi_debug/firmware.bin"
LITTLEFS="$PROJECTDIR/.pio/build/ulanzi_debug/littlefs.bin"
BOOT_APP0="$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
MERGED_BIN="$PROJECTDIR/.pio/build/ulanzi_debug/merged_firmware.bin"

# Check if the necessary binaries exist
if [ ! -f "$BOOTLOADER" ]; then
    echo "Bootloader not found. Please build it first."
    exit 1
fi

if [ ! -f "$PARTITIONS" ]; then
    echo "Partitions binary not found. Please build it first."
    exit 1
fi

if [ ! -f "$FIRMWARE" ]; then
    echo "Firmware not found. Please build it first."
    exit 1
fi

if [ ! -f "$LITTLEFS" ]; then
    echo "LittleFS binary not found. Please build it first."
    exit 1
fi

# Merge binaries using esptool.py
echo "Merging binaries into a single file..."

python ~/.platformio/packages/tool-esptoolpy/esptool.py \
    --chip esp32 merge_bin \
    --output "$MERGED_BIN" \
    0x1000 "$BOOTLOADER" \
    0x8000 "$PARTITIONS" \
    0xe000 "$BOOT_APP0" \
    0x10000 "$FIRMWARE" \
    0x210000 "$LITTLEFS"

if [ $? -ne 0 ]; then
    echo "Failed to merge binaries."
    exit 1
fi

echo "Merged binaries successfully created at $MERGED_BIN"
