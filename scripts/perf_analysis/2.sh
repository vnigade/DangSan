#!/bin/sh

set -e

source ~/Metalloc/autosetup-benchmarks.inc

: ${DUPLICATE_STR:="Number of duplicate pointers per object"}
: ${UNIQUE_STR:="Number of unique pointers per object"}
: ${LASTDUP_STR:="Number of duplicate pointers per object within"}

cd ~/Metalloc/autosetup.dir/llvm-apps/out-prun-spec/metaalloc-var-8b-iter1/out-prun-spec/metaalloc-var-8b-iter1/1

echo "Benchmark," "AVG Unique Ptrs," "AVG Duplicate Ptrsi," "AVG Duplicate Ptrs within Lookbehind" 

for bench in $BENCHMARKS; do
    declare -a avg_output 
    index=0
    for str in "$UNIQUE_STR" "$DUPLICATE_STR" "$LASTDUP_STR"; do
        if [ $index -eq 1 ]; then
           lines=`egrep "$str" ${bench}-out.0 | egrep -v "within" | awk '{print $NF}' | wc -l`
            total=`egrep "$str" ${bench}-out.0 | egrep -v "within" | awk '{print $NF}' | paste -sd+ - | bc`
        else
           lines=`egrep "$str" ${bench}-out.0 | awk '{print $NF}' | wc -l`
           total=`egrep "$str" ${bench}-out.0 | awk '{print $NF}' | paste -sd+ - | bc`
        fi
        #echo $str 
        #echo $lines 
        #echo $total
        avg_output[$index]=`echo "$total / $lines" | bc`
        index=$((index+1))
    done
    echo "$bench, ${avg_output[0]}, ${avg_output[1]}, ${avg_output[2]}"
done
