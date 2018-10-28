#!/bin/bash

: ${BENCHMARKS:="401.bzip2 403.gcc 433.milc 445.gobmk 456.hmmer 458.sjeng 462.libquantum 464.h264ref 470.lbm 482.sphinx3 400.perlbench 444.namd 447.dealII 471.omnetpp 473.astar 483.xalancbmk"}
#: ${BENCHMARKS:="400.perlbench"}

#CONFIG="metaalloc-logsize-0 metaalloc-logsize-1 metaalloc-logsize-2 metaalloc-logsize-3" BENCHMARKS=${BENCHMARKS} python ./1.py

CONFIG="baseline-iter1 metaalloc-var-8b-iter1" BENCHMARKS=${BENCHMARKS} python ./perf_num.py
