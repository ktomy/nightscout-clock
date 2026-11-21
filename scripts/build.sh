#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(dirname -- "$SCRIPT_DIR")
BUILD_MODE=firmware
UPLOAD=false
MONITOR=false

usage() {
    echo "Usage: $0 [--all] [--upload] [--monitor]"
    echo "  default    Build the firmware, with the settings page in it"
    echo "  --all      With --upload, also write the bootloader and partition table (see upload.sh)"
    echo "  --upload   Upload over USB"
    echo "  --monitor  Start the serial monitor after building/uploading"
    echo "To install over WiFi instead, use scripts/ota.sh."
}

while (($#)); do
    case $1 in
        --all)
            BUILD_MODE=all
            ;;
        --upload)
            UPLOAD=true
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

echo "Building firmware..."
if ! platformio run --environment ulanzi_debug --project-dir "$PROJECT_DIR"; then
    echo "ERROR: Firmware build failed." >&2
    exit 1
fi

if [[ $UPLOAD == true ]]; then
    upload_arguments=()
    if [[ $BUILD_MODE == all ]]; then
        upload_arguments+=(--all)
    fi
    if [[ $MONITOR == true ]]; then
        upload_arguments+=(--monitor)
    fi
    # macOS ships bash 3.2, where expanding an empty array under "set -u" is an error.
    if ! bash "$SCRIPT_DIR/upload.sh" ${upload_arguments[@]+"${upload_arguments[@]}"}; then
        echo "ERROR: Upload workflow failed." >&2
        exit 1
    fi
elif [[ $MONITOR == true ]]; then
    if ! bash "$SCRIPT_DIR/monitor.sh"; then
        echo "ERROR: Serial monitor failed." >&2
        exit 1
    fi
fi
