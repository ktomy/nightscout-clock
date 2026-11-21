#!/usr/bin/env bash
# Install a local build on the clock over WiFi, the way the settings page installs an update: the clock
# stops its data source, shows UPDATE, takes the firmware into its other slot, and restarts on it.
#
# The clock's address is --url, or the git-ignored .local_clock_ip file (scripts/clock_address.py).
# When the settings page has a password, give it with --password or in NSCLOCK_PASSWORD.
#
# Usage:
#   scripts/ota.sh [--release] [--no-build] [--url <address>] [--password <password>]
#
#   --release   Install the release build (ulanzi) instead of the debug build (ulanzi_debug)
#   --no-build  Install the last build as it is
#
# Needs bash, curl and python3.

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PROJECT_DIR=$(dirname -- "$SCRIPT_DIR")
ENVIRONMENT=ulanzi_debug
BUILD=true
URL=""
PASSWORD=${NSCLOCK_PASSWORD:-}

usage() {
    sed -n '8,13p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

fail() {
    echo "ERROR: $*" >&2
    exit 1
}

while (($#)); do
    case $1 in
        --release)
            ENVIRONMENT=ulanzi
            ;;
        --no-build)
            BUILD=false
            ;;
        --url)
            (($# >= 2)) || fail "--url needs an address."
            URL=$2
            shift
            ;;
        --password)
            (($# >= 2)) || fail "--password needs a value."
            PASSWORD=$2
            shift
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

if [[ $BUILD == true ]]; then
    echo "Building $ENVIRONMENT..."
    platformio run --environment "$ENVIRONMENT" --project-dir "$PROJECT_DIR" || fail "Firmware build failed."
fi

FIRMWARE="$PROJECT_DIR/.pio/build/$ENVIRONMENT/firmware.bin"
[[ -f $FIRMWARE ]] || fail "Missing $FIRMWARE; build it first."
# "<version>:<release|debug>", from the marker the clock checks (src/FirmwareUpdateManager.cpp).
EXPECTED=$(grep -ao 'NSCLOCK-OTA-1:[0-9A-Za-z.-]*:[a-z]*' "$FIRMWARE" | head -n 1 | cut -d: -f2-) || true
[[ -n $EXPECTED ]] || fail "$FIRMWARE has no update marker; it cannot be installed over WiFi."
SIZE=$(wc -c < "$FIRMWARE" | tr -d '[:space:]')

CLOCK=$(python3 -c '
import sys
sys.path.insert(0, sys.argv[1])
from clock_address import resolve_clock_url
print(resolve_clock_url(sys.argv[2] or None) or "")
' "$SCRIPT_DIR" "$URL")
[[ -n $CLOCK ]] || fail "No clock address: pass --url or put the address in .local_clock_ip."

WORK_DIR=$(mktemp -d)
trap 'rm -rf "$WORK_DIR"' EXIT
COOKIES="$WORK_DIR/cookies"
RESPONSE="$WORK_DIR/response"
touch "$COOKIES"

# Prints the HTTP status; the body goes to $RESPONSE.
clock_request() {
    curl --silent --show-error --cookie "$COOKIES" --cookie-jar "$COOKIES" \
        --output "$RESPONSE" --write-out '%{http_code}' "$@" || true
}

json_field() {
    python3 -c '
import json, sys
try:
    print(json.load(open(sys.argv[1])).get(sys.argv[2], ""))
except Exception:
    print("")
' "$RESPONSE" "$1"
}

if [[ -n $PASSWORD ]]; then
    # From the environment, so the password does not show in the process list.
    BODY=$(NSCLOCK_PASSWORD=$PASSWORD python3 -c 'import json, os; print(json.dumps({"password": os.environ["NSCLOCK_PASSWORD"]}))')
    status=$(clock_request --max-time 10 -H 'Content-Type: application/json' --data-binary "$BODY" \
        "${CLOCK}api/auth/login")
    [[ $status == 200 ]] || fail "Login failed (HTTP $status). Check the password."
fi

echo "Installing ${EXPECTED%%:*} (${EXPECTED##*:}, $SIZE bytes) on $CLOCK"
status=$(clock_request --max-time 10 -X POST -H 'Content-Type: application/json' --data '{}' \
    "${CLOCK}api/update/start")
case $status in
    202) ;;
    401) fail "The clock asks for a login: pass --password or set NSCLOCK_PASSWORD." ;;
    409) fail "The clock is already installing an update." ;;
    000) fail "Could not reach the clock at $CLOCK." ;;
    *) fail "The clock refused to start the update (HTTP $status)." ;;
esac

echo "Waiting for the clock to stop its data source..."
for ((i = 0; ; i++)); do
    status=$(clock_request --max-time 5 "${CLOCK}api/update/status")
    state=$(json_field state)
    [[ $state == ready ]] && break
    if [[ $state == failed ]] || ((i >= 55)); then
        fail "The clock did not get ready for the update; it restarts on its current firmware."
    fi
    sleep 1
done

echo "Uploading..."
status=$(curl --cookie "$COOKIES" --progress-bar --max-time 600 --output "$RESPONSE" --write-out '%{http_code}' \
    -H "X-Update-Size: $SIZE" -F "firmware=@$FIRMWARE;type=application/octet-stream" \
    "${CLOCK}api/update") || status=000
if [[ $status != 200 ]]; then
    error=$(json_field error)
    fail "The upload failed (HTTP $status${error:+: $error}). The clock shows FAILED and restarts on its current firmware."
fi

echo "Installed. Waiting for the clock to restart..."
sleep 5
for ((i = 0; ; i++)); do
    if version=$(curl --silent --fail --max-time 3 "${CLOCK}version.txt"); then
        break
    fi
    ((i < 60)) || fail "The clock has not come back after two minutes."
    sleep 2
done

status=$(clock_request --max-time 10 "${CLOCK}api/status")
debug=$(json_field debug)
kind=$([[ $debug == True ]] && echo debug || echo release)
echo "The clock runs $version ($kind)."
if [[ "$version:$kind" != "$EXPECTED" ]]; then
    fail "Expected ${EXPECTED%%:*} (${EXPECTED##*:}): the new firmware may not have started, and the clock went back to the previous one."
fi
