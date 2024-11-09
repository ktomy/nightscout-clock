#!/bin/bash

if [[ $PWD == *"scripts"* ]]; then
    PROJECTDIR=$(dirname $PWD)
else
    PROJECTDIR=$PWD
fi

platformio device monitor  --project-dir $PROJECTDIR --environment ulanzi_debug