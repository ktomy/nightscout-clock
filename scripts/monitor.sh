#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(dirname -- "$SCRIPT_DIR")

# shellcheck source=get_port.sh
source "$SCRIPT_DIR/get_port.sh"
load_platformio_config
MONITOR_PORT=$(get_port)

MONITOR_FILTERS=$(get_platformio_option monitor_filters)
FILTER_ARGS=()
while IFS= read -r filter; do
    if [[ -n $filter && $filter != log2file ]]; then
        FILTER_ARGS+=(--filter "$filter")
    fi
done <<<"$MONITOR_FILTERS"
FILTER_ARGS+=(--filter log2file)

echo "Monitoring $MONITOR_PORT; saving output under $PROJECT_DIR/logs/"
if ! platformio device monitor --port "$MONITOR_PORT" --project-dir "$PROJECT_DIR" \
    --environment ulanzi_debug "${FILTER_ARGS[@]}"; then
    echo "ERROR: Serial monitor exited with an error." >&2
    exit 1
fi
