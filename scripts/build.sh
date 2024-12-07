#!/bin/bash

if [[ $PWD == *"scripts"* ]]; then
    PROJECTDIR=$(dirname $PWD)
else
    PROJECTDIR=$PWD
fi

if [[ $1 != "--fs" ]]; then
    echo "Building firmware..."
    if ! platformio run --environment ulanzi_debug --project-dir $PROJECTDIR; then
        echo "Build failed"
        exit 1
    fi
fi

if [[ $1 == "--fs" || $1 == "--all" ]]; then
    echo "Building LittleFS..."
    if ! platformio run --target buildfs --environment ulanzi_debug --project-dir $PROJECTDIR; then
        echo "LittleFS build failed"
        exit 1
    fi
fi

if [[ $1 == "--upload" || $2 == "--upload" ]]; then
    if [[ $1 == "--fs" || $1 == "--fs" ]]; then
        echo "Uploading LittleFS..."
        $PROJECTDIR/scripts/upload.sh --fs
    else
        $PROJECTDIR/scripts/upload.sh $1 $2 $3
    fi
fi