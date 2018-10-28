#!/bin/bash

set -e

source ./autosetup-benchmarks.inc

: ${CONFIG:="$1"}
: ${CONFIGNAME:="$CONFIG"}
: ${PRUNITER:=4}
: ${PRUNNODES:=4}
: ${TIMEOUTS:=7200}

case "$CONFIG" in
default|baseline|metaalloc|baselinesafestack)
	;;
*)
	echo "Bad config \"$CONFIG\"" >&2
	exit 1
	;;
esac

cd autosetup.dir/llvm-apps
for i in `seq 1 "$PRUNITER"`; do
	echo "`date +%FT%T`: iteration $i of $PRUNITER"
	outdir="out-prun-spec/$CONFIGNAME/$i"
	mkdir -p "$outdir"
	for bench in $BENCHMARKS; do
		echo "`date +%FT%T`: benchmark $bench"
		outfile="$outdir/$bench-out"
		C="$bench" prun -np "$PRUNNODES" -t "$TIMEOUTS" -o "$outfile" -1 "./run-spec-$CONFIG.sh" || echo "job failed; outfile=$outfile, status=$?"
	done
	find "$outdir" -name "*-out.*" -print0 | xargs -0 grep runbench_secs > "$outdir-times.txt" || true
	find "$outdir" -name "*-out.*" -print0 | xargs -0 grep -c runbench_secs | grep -v ':1$' > "$outdir-bad.txt" || true
	tar cf - "$outdir" | bzip2 > "$outdir.tar.bz2"
	rm -r "$outdir"
done
