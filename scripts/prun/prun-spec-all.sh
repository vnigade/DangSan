#!/bin/bash

set -e

: ${PATHROOT="$PWD"}
: ${PRUNITER=1}
: ${PRUNOPTS=""}
: ${PRUNNODES=32}
: ${PRUNPERNODE=1}

: ${BENCHMARKS="
	400.perlbench
	401.bzip2
	403.gcc
	433.milc
	445.gobmk
	456.hmmer
	458.sjeng
	462.libquantum
	464.h264ref
	470.lbm
	482.sphinx3"}
#429.mcf

if [ ! -f "$PATHROOT/scripts/prun/prun-spec.sh" ]; then
	echo "Please run from MetaAlloc repository root or set \$PATHROOT" >&2
	exit 1
fi

export BENCHMARK
export INSTANCE
export PATHROOT
export PRUNITE
export PRUNOPTS
export PRUNNODES
export PRUNPERNODE

for BENCHMARK in $BENCHMARKS; do
#for BENCHMARK in 403.gcc; do
for INSTANCE in metaalloc baseline default; do
	echo "benchmark=$BENCHMARK instance=$INSTANCE"
	"$PATHROOT/scripts/prun/prun-spec.sh"
done
done
