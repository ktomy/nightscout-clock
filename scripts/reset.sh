#!/bin/bash

if [[ $PWD == *"scripts"* ]]; then
    PROJECTDIR=$(dirname $PWD)
else
    PROJECTDIR=$PWD
fi

# Read a PlatformIO setting, preferring the git-ignored per-machine override
# (platformio.local.ini) and falling back to the tracked platformio.ini.
# See platformio.local.ini.example for how to create the local file.
# On a match it prints "value|sourcefile"; empty output means not found.
CONFIG_FILES=("$PROJECTDIR/platformio.local.ini" "$PROJECTDIR/platformio.ini")
read_ini() { # $1 = key name, e.g. upload_port
    local key=$1 file value
    for file in "${CONFIG_FILES[@]}"; do
        [[ -f $file ]] || continue
        value=$(awk -F '=' -v k="$key" '$1 ~ "^"k"[ \t]*$" {gsub(/[ \t]/, "", $2); print $2; exit}' "$file")
        if [[ -n $value ]]; then
            echo "$value|$(basename "$file")"
            return
        fi
    done
}

echo "Reading upload configuration (checking platformio.local.ini, then platformio.ini)..."
PORT_RESULT=$(read_ini upload_port)
UPLOAD_PORT=${PORT_RESULT%%|*}
UPLOAD_PORT_SOURCE=${PORT_RESULT#*|}
SPEED_RESULT=$(read_ini upload_speed)
UPLOAD_SPEED=${SPEED_RESULT%%|*}
UPLOAD_SPEED_SOURCE=${SPEED_RESULT#*|}

if [[ -z "$UPLOAD_PORT" ]]; then
    echo "ERROR: upload_port not set. Copy platformio.local.ini.example to platformio.local.ini and set upload_port (find it with: pio device list)."
    exit 1
fi

if [[ -z "$UPLOAD_SPEED" ]]; then
    echo "ERROR: upload_speed not set in platformio.local.ini or platformio.ini"
    exit 1
fi

echo "  upload_port  = $UPLOAD_PORT (from $UPLOAD_PORT_SOURCE)"
echo "  upload_speed = $UPLOAD_SPEED (from $UPLOAD_SPEED_SOURCE)"

while true; do
    python ~/.platformio/packages/tool-esptoolpy/esptool.py \
        --chip esp32 --port "$UPLOAD_PORT" --baud "$UPLOAD_SPEED" --before default_reset --after hard_reset \
        run
    if [ $? -eq 0 ]; then
        break
    fi
done

echo "Reset done"
