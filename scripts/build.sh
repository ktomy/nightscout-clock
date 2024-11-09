#!/bin/bash

# @echo off
# cd ..
# C:\Users\User\.platformio\penv\Scripts\platformio.exe run --environment ulanzi_debug
# cd Scripts
# echo "Build success"


if [[ $PWD == *"scripts"* ]]; then
    PROJECTDIR=$(dirname $PWD)
else
    PROJECTDIR=$PWD
fi

if [[ $1 != "--fs" ]]; then
    echo "Building firmware..."
    if ! platformio run --environment ulanzi_debug; then
        echo "Build failed"
        exit 1
    fi
fi

if [[ $1 == "--fs" || $1 == "--all" ]]; then
    echo "Building LittleFS..."
    if ! platformio run --target buildfs --environment ulanzi_debug; then
        echo "LittleFS build failed"
        exit 1
    fi
fi

if [[ $1 == "--upload" || $2 == "--upload" ]]; then
    $PROJECTDIR/scripts/upload.sh $1 $2 $3
fi