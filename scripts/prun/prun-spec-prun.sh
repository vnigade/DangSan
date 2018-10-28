#!/bin/bash

set -e

# expected incoming parameters:
# - BENCHMARK
# - INSTANCE
# - PATHROOT
# - PRUN_CPU_RANK (from prun)

# run the job
echo "Starting QEMU for test $PRUN_CPU_RANK"

C="$BENCHMARK" "$PATHROOT/autosetup.dir/llvm-apps/run-spec-$INSTANCE.sh"

echo "Done with test $PRUN_CPU_RANK"
