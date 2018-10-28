#!/bin/bash

set -e

: ${PRUNALLITER:=4}
: ${PRUNITER:=1}
: ${PRUNNODES:=4}

export CONFIG_FIXEDCOMPRESSION=false
export CONFIG_METADATABYTES=8
export CONFIG_DEEPMETADATA=false
export CONFIG_DEEPMETADATABYTES=8
export CONFIGNAME
export PRUNITER
export PRUNNODES

run()
{
	echo "Building $CONFIGNAME"
	INSTANCES=$CONFIG ./autosetup.sh
	echo "Running $CONFIGNAME"
	./autosetup-prun.sh "$CONFIG"
}

for i in `seq 1 "$PRUNALLITER"`; do
#CONFIGNAME=metaalloc-logsize-$LOGSIZE CONFIG=metaalloc run
CONFIGNAME=metaalloc-var-8b-iter$i CONFIG=metaalloc run
CONFIGNAME=baseline-iter$i CONFIG=baseline run
CONFIGNAME=baselinesafestack-iter$i CONFIG=baselinesafestack run
#CONFIGNAME=default-iter$i CONFIG=default run
#CONFIGNAME=metaalloc-var-1b-iter$i CONFIG=metaalloc CONFIG_METADATABYTES=1 run
#CONFIGNAME=metaalloc-fix-1b-iter$i CONFIG=metaalloc CONFIG_METADATABYTES=1 CONFIG_FIXEDCOMPRESSION=true run
#CONFIGNAME=metaalloc-fix-8b-iter$i CONFIG=metaalloc CONFIG_METADATABYTES=8 CONFIG_FIXEDCOMPRESSION=true run
done
