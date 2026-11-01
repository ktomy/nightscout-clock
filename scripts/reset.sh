#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
MAX_RESET_ATTEMPTS=10

if (($#)); then
    if [[ $1 == -h || $1 == --help ]]; then
        echo "Usage: $0"
        exit
    fi
    echo "ERROR: Unknown argument: $1" >&2
    exit 2
fi

# shellcheck source=get_port.sh
source "$SCRIPT_DIR/get_port.sh"
load_platformio_config
UPLOAD_PORT=$(get_port)
UPLOAD_SPEED=$(get_platformio_option upload_speed)

if [[ ! $UPLOAD_SPEED =~ ^[0-9]+$ ]]; then
    echo "ERROR: Invalid upload_speed in the resolved PlatformIO configuration: $UPLOAD_SPEED" >&2
    exit 1
fi

echo "Resetting $UPLOAD_PORT at $UPLOAD_SPEED baud..."
for ((attempt = 1; attempt <= MAX_RESET_ATTEMPTS; attempt++)); do
    if python ~/.platformio/packages/tool-esptoolpy/esptool.py \
        --chip esp32 --port "$UPLOAD_PORT" --baud "$UPLOAD_SPEED" \
        --before default_reset --after hard_reset run; then
        echo "Reset done"
        exit
    fi

    if ((attempt == MAX_RESET_ATTEMPTS)); then
        echo "ERROR: Reset failed after $MAX_RESET_ATTEMPTS attempts." >&2
        exit 1
    fi
    echo "Reset failed. Retrying ($((attempt + 1))/$MAX_RESET_ATTEMPTS)..." >&2
    sleep 1
done
