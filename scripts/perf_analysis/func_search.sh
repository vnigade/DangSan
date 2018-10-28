#!/bin/bash

SRC_DIR="/home/vne500/CPU2006/403.gcc/src"

for func in `cat /home/vne500/tmp.diff`; do
    grep "^${func}" ${SRC_DIR}/* > /dev/null 2>&1
    ret=$?
    if [ "$ret" -ne 0 ]; then
        echo "Function: ${func} not found"
        exit
    fi
done
