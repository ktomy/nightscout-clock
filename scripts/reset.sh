#!/bin/bash

while true; do
    python ~/.platformio/packages/tool-esptoolpy/esptool.py \
        --chip esp32 --port "/dev/ttyUSB0" --baud 921600 --before default_reset --after hard_reset \
        run
    if [ $? -eq 0 ]; then
        break
    fi
done

echo "Reset done"