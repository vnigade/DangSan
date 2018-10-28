#!/bin/bash

set -e
source ~/Metalloc/autosetup-benchmarks.inc

: ${CONFIG:="metaalloc-var-8b baseline"}
: ${ITER:=1}
: ${VSZ:="runbench_mean_vsz"}
: ${PSS:="runbench_mean_pss"}
: ${RSS:="runbench_mean_rss"}
: ${META_ALLOC_PATH:="/home/vne500/Metalloc/autosetup.dir/llvm-apps/out-prun-spec/metaalloc-var-8b-iter1/out-prun-spec/metaalloc-var-8b-iter1/1"}
: ${BASELINE_PATH:="/home/vne500/Metalloc/autosetup.dir/llvm-apps/out-prun-spec/baseline-iter1/out-prun-spec/baseline-iter1/1"}
: ${SAFESTACK_PATH:="/home/vne500/Metalloc/autosetup.dir/llvm-apps/out-prun-spec/baselinesafestack-iter1/out-prun-spec/baselinesafestack-iter1/1"}

cd ~/Metalloc/autosetup.dir/llvm-apps/out-prun-spec/

echo "Benchmark," "Baseline-vsz," "Baseline-pss," "Baseline-rss," "DangSan-vsz," "DangSan-pss," "DangSan-rss"

for bench in $BENCHMARKS; do
    bvsz=`grep "${VSZ}" ${BASELINE_PATH}/${bench}-out.0 | awk -F'=' '{print $2/1024}'`
    bpss=`grep "${PSS}" ${BASELINE_PATH}/${bench}-out.0 | awk -F'=' '{print $2/1024}'`
    brss=`grep "${RSS}" ${BASELINE_PATH}/${bench}-out.0 | awk -F'=' '{print $2/1024}'`
    
    #svsz=`grep "${VSZ}" ${SAFESTACK_PATH}/${bench}-out.0 | awk -F'=' '{print $2}'`
    #spss=`grep "${PSS}" ${SAFESTACK_PATH}/${bench}-out.0 | awk -F'=' '{print $2}'`
    #srss=`grep "${RSS}" ${SAFESTACK_PATH}/${bench}-out.0 | awk -F'=' '{print $2}'`
    
    dvsz=`grep "${VSZ}" ${META_ALLOC_PATH}/${bench}-out.0 | awk -F'=' '{print $2/1024}'`
    dpss=`grep "${PSS}" ${META_ALLOC_PATH}/${bench}-out.0 | awk -F'=' '{print $2/1024}'`
    drss=`grep "${RSS}" ${META_ALLOC_PATH}/${bench}-out.0 | awk -F'=' '{print $2/1024}'`

    echo "$bench,$bvsz,$bpss,$brss,$dvsz,$dpss,$drss"
done
