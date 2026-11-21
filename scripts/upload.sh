#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(dirname -- "$SCRIPT_DIR")
UPLOAD_MODE=firmware
MONITOR=false
MAX_UPLOAD_ATTEMPTS=10

usage() {
    echo "Usage: $0 [--all] [--monitor]"
    echo "  default    Upload the firmware into the first slot and boot it"
    echo "  --all      Also upload the bootloader and partition table, for a clock with an older layout"
    echo "  --monitor  Start the serial monitor after a successful upload"
}

while (($#)); do
    case $1 in
        --all)
            UPLOAD_MODE=all
            ;;
        --monitor)
            MONITOR=true
            ;;
        -h | --help)
            usage
            exit
            ;;
        *)
            echo "ERROR: Unknown argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
    shift
done

# shellcheck source=get_port.sh
source "$SCRIPT_DIR/get_port.sh"
load_platformio_config
UPLOAD_PORT=$(get_port)
UPLOAD_SPEED=$(get_platformio_option upload_speed)

if [[ ! $UPLOAD_SPEED =~ ^[0-9]+$ ]]; then
    echo "ERROR: Invalid upload_speed in the resolved PlatformIO configuration: $UPLOAD_SPEED" >&2
    exit 1
fi

BUILD_DIR="$PROJECT_DIR/.pio/build/ulanzi_debug"
flash_arguments=()

require_artifact() {
    if [[ ! -f $1 ]]; then
        echo "ERROR: Missing artifact: $1" >&2
        echo "Build the requested target before uploading." >&2
        exit 1
    fi
}

# The firmware goes into the first slot, and boot_app0 resets the OTA data so the clock boots it: after an
# update from the settings page, the clock may be running the second slot.
BOOT_APP0_PATH=~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin
require_artifact "$BUILD_DIR/firmware.bin"
require_artifact "$BOOT_APP0_PATH"
flash_arguments=(0xe000 "$BOOT_APP0_PATH" 0x10000 "$BUILD_DIR/firmware.bin")

if [[ $UPLOAD_MODE == all ]]; then
    require_artifact "$BUILD_DIR/bootloader.bin"
    require_artifact "$BUILD_DIR/partitions.bin"
    flash_arguments=(
        0x1000 "$BUILD_DIR/bootloader.bin"
        0x8000 "$BUILD_DIR/partitions.bin"
        "${flash_arguments[@]}"
    )
fi

echo "Uploading to $UPLOAD_PORT at $UPLOAD_SPEED baud..."
for ((attempt = 1; attempt <= MAX_UPLOAD_ATTEMPTS; attempt++)); do
    if python ~/.platformio/packages/tool-esptoolpy/esptool.py \
        --chip esp32 --port "$UPLOAD_PORT" --baud "$UPLOAD_SPEED" \
        --before default_reset --after hard_reset \
        write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB \
        "${flash_arguments[@]}"; then
        echo "Upload done"
        break
    fi

    if ((attempt == MAX_UPLOAD_ATTEMPTS)); then
        echo "ERROR: Upload failed after $MAX_UPLOAD_ATTEMPTS attempts." >&2
        exit 1
    fi
    echo "Upload failed. Retrying ($((attempt + 1))/$MAX_UPLOAD_ATTEMPTS)..." >&2
    sleep 1
done

if [[ $MONITOR == true ]]; then
    sleep 2
    if ! bash "$SCRIPT_DIR/monitor.sh"; then
        echo "ERROR: Serial monitor failed after upload." >&2
        exit 1
    fi
fi
