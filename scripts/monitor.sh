#!/bin/bash

if [[ $PWD == *"scripts"* ]]; then
    PROJECTDIR=$(dirname $PWD)
else
    PROJECTDIR=$PWD
fi

LOGFILE="$PROJECTDIR/log/monitor_$(date +'%Y%m%d_%H%M%S').log"

platformio device monitor --project-dir "$PROJECTDIR" --environment ulanzi_debug | tee "$LOGFILE"