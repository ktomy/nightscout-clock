#!/usr/bin/env bash
# Build the firmware and a LittleFS image for the Wokwi simulator, then run the simulation.
#
# The filesystem image is built from data/ without your data/config.json (see
# scripts/wokwi_data.py). Its config.json is data/config_initial.json with the values in
# scripts/wokwi_config.json applied: the simulator's open "Wokwi-GUEST" Wi-Fi network and the
# "API" data source. wokwi-cli cannot reach the clock over HTTP, so tests inject readings and read
# the LED matrix through serial commands (see src/WokwiConsole.h), usually from a scenario in
# scripts/wokwi_scenarios/.
#
# wokwi-cli must be installed (https://docs.wokwi.com/wokwi-ci/cli-installation) and a CI token from
# https://wokwi.com/dashboard/ci must be set in WOKWI_CLI_TOKEN or saved in the git-ignored
# .wokwi_token file in the repository root.
#
# Usage:
#   scripts/wokwi.sh [--no-build] [wokwi-cli options...]
#
# Examples:
#   scripts/wokwi.sh --timeout 120000 --scenario scripts/wokwi_scenarios/smoke.yaml \
#       --serial-log-file logs/wokwi.log
#   python3 scripts/wokwi_screen.py logs/wokwi.log --ascii   # LED matrix dumps -> PNG files
#
# Any option after --no-build is passed to wokwi-cli unchanged. --timeout is simulated time
# (default 30 s); the simulation runs at roughly half of real-time speed.

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(dirname -- "$SCRIPT_DIR")
ENVIRONMENT=wokwi
BUILD_DIR="$PROJECT_DIR/.pio/build/$ENVIRONMENT"
BUILD=true

if [[ ${1:-} == --no-build ]]; then
    BUILD=false
    shift
fi

WOKWI_CLI=$(command -v wokwi-cli || true)
if [[ -z $WOKWI_CLI && -x $HOME/.wokwi/bin/wokwi-cli ]]; then
    WOKWI_CLI=$HOME/.wokwi/bin/wokwi-cli
fi
if [[ -z $WOKWI_CLI ]]; then
    echo "ERROR: wokwi-cli not found. See https://docs.wokwi.com/wokwi-ci/cli-installation" >&2
    exit 1
fi

if [[ -z ${WOKWI_CLI_TOKEN:-} && -f $PROJECT_DIR/.wokwi_token ]]; then
    WOKWI_CLI_TOKEN=$(tr -d '[:space:]' < "$PROJECT_DIR/.wokwi_token")
    export WOKWI_CLI_TOKEN
fi
if [[ -z ${WOKWI_CLI_TOKEN:-} ]]; then
    echo "ERROR: No Wokwi CI token. Get one at https://wokwi.com/dashboard/ci and put it in" >&2
    echo "WOKWI_CLI_TOKEN or in the .wokwi_token file in the repository root." >&2
    exit 1
fi

if [[ $BUILD == true ]]; then
    echo "Building firmware..."
    platformio run --environment "$ENVIRONMENT" --project-dir "$PROJECT_DIR"
    echo "Building LittleFS..."
    platformio run --target buildfs --environment "$ENVIRONMENT" --project-dir "$PROJECT_DIR"
    bash "$SCRIPT_DIR/merge_bins.sh" "$ENVIRONMENT"
fi

if [[ ! -f $BUILD_DIR/merged_firmware.bin ]]; then
    echo "ERROR: $BUILD_DIR/merged_firmware.bin not found; run without --no-build first." >&2
    exit 1
fi

exec "$WOKWI_CLI" "$@" "$PROJECT_DIR"
