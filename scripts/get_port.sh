#!/usr/bin/env bash

# This file can be executed to print the configured port, or sourced by another
# helper script to reuse get_port and get_platformio_option.

NSCLOCK_CONFIG_SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
NSCLOCK_PROJECT_DIR=$(dirname -- "$NSCLOCK_CONFIG_SCRIPT_DIR")
NSCLOCK_PLATFORMIO_ENVIRONMENT=ulanzi_debug

load_platformio_config() {
    if [[ ${NSCLOCK_PLATFORMIO_CONFIG_LOADED:-false} == true ]]; then
        return
    fi

    if ! NSCLOCK_PLATFORMIO_CONFIG_JSON=$(
        PLATFORMIO_SETTING_ENABLE_TELEMETRY=no \
            platformio project config --project-dir "$NSCLOCK_PROJECT_DIR" --json-output
    ); then
        echo "ERROR: Could not resolve the PlatformIO project configuration." >&2
        return 1
    fi

    NSCLOCK_PLATFORMIO_CONFIG_LOADED=true
}

get_platformio_option() {
    local option=$1

    load_platformio_config || return 1
    if ! python -c '
import json
import sys

sections = {name: dict(options) for name, options in json.load(sys.stdin)}
value = sections.get(f"env:{sys.argv[1]}", {}).get(sys.argv[2], "")
if isinstance(value, list):
    value = "\n".join(str(item) for item in value)
sys.stdout.write(str(value))
' "$NSCLOCK_PLATFORMIO_ENVIRONMENT" "$option" <<<"$NSCLOCK_PLATFORMIO_CONFIG_JSON"
    then
        echo "ERROR: Could not read '$option' from the resolved PlatformIO configuration." >&2
        return 1
    fi
}

get_port() {
    local port

    port=$(get_platformio_option upload_port) || return 1
    if [[ -z $port ]]; then
        echo "ERROR: No upload_port is configured." >&2
        echo "Copy platformio.local.ini.example to platformio.local.ini and set upload_port." >&2
        return 1
    fi

    printf '%s\n' "$port"
}

if [[ ${BASH_SOURCE[0]} == "$0" ]]; then
    set -euo pipefail
    get_port
fi
