#!/bin/bash

set -e

: ${PATHROOT:="$PWD"}
: ${LOG_SIZE:="0 1 2 3"}
: ${BENCHMARKS:="401.bzip2 403.gcc 433.milc 445.gobmk 456.hmmer 458.sjeng 462.libquantum 464.h264ref 470.lbm 482.sphinx3 400.perlbench 444.namd 447.dealII 471.omnetpp 473.astar 483.xalancbmk"}

# Clean and build static lib
for log_size in ${LOG_SIZE};
do
    # Clean and build static lib
    echo "Building staticlib with $log_size logsize.."
    cd $PATHROOT/staticlib
    make clean
    #make TC_ALLOC=1 DANG_LOG_SIZE=$log_size
    make TC_ALLOC=1 DANG_NLOOKBEHIND=$log_size
    
    # run autsetup-pruna-all.sh
    echo "Running SPEC for $log_size log_size.."
    cd $PATHROOT
    LOGSIZE=$log_size PRUNALLITER=1 BENCHMARKS=${BENCHMARKS} ./autosetup-prun-all.sh
done
echo "Run Completed!"
