#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(dirname -- "$SCRIPT_DIR")
BUILD_MODE=firmware
UPLOAD=false
MONITOR=false

usage() {
    echo "Usage: $0 [--fs | --all] [--upload] [--monitor]"
    echo "  default    Build firmware"
    echo "  --fs       Build only LittleFS"
    echo "  --all      Build firmware and LittleFS"
    echo "  --upload   Upload the artifacts selected above"
    echo "  --monitor  Start the serial monitor after building/uploading"
}

while (($#)); do
    case $1 in
        --fs | --all)
            requested_mode=${1#--}
            if [[ $BUILD_MODE != firmware && $BUILD_MODE != "$requested_mode" ]]; then
                echo "ERROR: --fs and --all cannot be used together." >&2
                exit 2
            fi
            BUILD_MODE=$requested_mode
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

if [[ $BUILD_MODE == firmware || $BUILD_MODE == all ]]; then
    echo "Building firmware..."
    if ! platformio run --environment ulanzi_debug --project-dir "$PROJECT_DIR"; then
        echo "ERROR: Firmware build failed." >&2
        exit 1
    fi
fi

if [[ $BUILD_MODE == fs || $BUILD_MODE == all ]]; then
    echo "Building LittleFS..."
    if ! platformio run --target buildfs --environment ulanzi_debug --project-dir "$PROJECT_DIR"; then
        echo "ERROR: LittleFS build failed." >&2
        exit 1
    fi
fi

if [[ $UPLOAD == true ]]; then
    upload_arguments=()
    if [[ $BUILD_MODE == fs ]]; then
        upload_arguments+=(--fs)
    elif [[ $BUILD_MODE == all ]]; then
        upload_arguments+=(--all)
    fi
    if [[ $MONITOR == true ]]; then
        upload_arguments+=(--monitor)
    fi
    if ((${#upload_arguments[@]})); then
        bash "$SCRIPT_DIR/upload.sh" "${upload_arguments[@]}" || upload_failed=true
    else
        bash "$SCRIPT_DIR/upload.sh" || upload_failed=true
    fi
    if [[ ${upload_failed:-false} == true ]]; then
        echo "ERROR: Upload workflow failed." >&2
        exit 1
    fi
elif [[ $MONITOR == true ]]; then
    if ! bash "$SCRIPT_DIR/monitor.sh"; then
        echo "ERROR: Serial monitor failed." >&2
        exit 1
    fi
fi
