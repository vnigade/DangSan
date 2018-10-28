#!/bin/bash

set -e

# script settings
: ${BENCHMARK:=all_c}    # SPEC2006 benchmark name/set
: ${INSTANCE:=metaalloc} # llvm-apps instance: default, baseline or metaalloc
: ${PATHROOT="$PWD"}     # path to MetaAlloc repository root
: ${PRUNITER:=1}         # number of times to invoke prun
: ${PRUNOPTS:=""}        # options to specify to prun (like "-t 30:00" to extend reservation duration)
: ${PRUNNODES:=1}        # number of nodes to use with prun
: ${PRUNPERNODE:=1}      # number of instances per node with prun

: ${PATHOUTBASE:="/var/scratch/$USER/results"}
: ${PATHOUT:="$PATHOUTBASE/spec-$INSTANCE-$BENCHMARK-`date '+%Y%m%d-%H%M%S'`"}
: ${PRUNSCRIPT:="$PATHROOT/scripts/prun/prun-spec-prun.sh"}

if [ ! -f "$PATHROOT/autosetup.sh" ]; then
	echo "Please run from MetaAlloc repository root or set \$PATHROOT" >&2
	exit 1
fi
if [ ! -f "$PATHROOT/autosetup.dir/llvm-apps/run-spec-$INSTANCE.sh" ]; then
	echo "llvm-apps instance not set up, running autosetup.sh (this will take a while)"
	cd "$PATHROOT"
	./autosetup.sh
fi 
if [ ! -f "$PATHROOT/autosetup.dir/llvm-apps/run-spec-$INSTANCE.sh" ]; then
	echo "It seems \"$INSTANCE\" is not a valid llvm-apps instance" >&2
	exit 1
fi

# print settings
echo "Running SPEC experiment with the following settings:"
echo "- BENCHMARK       = $BENCHMARK"
echo "- INSTANCE        = $INSTANCE"
echo "- PATHOUT         = $PATHOUT"
echo "- PRUNITER        = $PRUNITER"
echo "- PRUNOPTS        = $PRUNOPTS"
echo "- PRUNNODES       = $PRUNNODES"
echo "- PRUNPERNODE     = $PRUNPERNODE"
	
# prepare output directory
echo "Writing output to $PATHOUT"
mkdir -p "$PATHOUTBASE"
mkdir "$PATHOUT"
set > "$PATHOUT/settings.txt"

# run experiment
echo "Starting experiments"
export BENCHMARK
export INSTANCE
export PATHROOT
cd "$PATHOUT"
for iter in `seq 1 "$PRUNITER"`; do
	echo "Starting experiment $iter out of $PRUNITER"
	prun -export-env -np "$PRUNNODES" "-$PRUNPERNODE" -o "script-out-$iter" $PRUNOPTS "$PRUNSCRIPT" > "$PATHOUT/prun-out-$iter.txt" 2>&1 || true
done

echo "Completed succesfully"
