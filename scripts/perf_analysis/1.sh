#!/bin/bash

set -x
set -e

source ~/Metalloc/autosetup-benchmarks.inc

#: ${CONFIG:="metaalloc-var-8b baseline baselinesafestack"}
: ${CONFIG:="metaalloc-var-8b baseline"}
: ${PRUNITER:=4}
: ${ITER:=1}

cd ~/Metalloc/autosetup.dir/llvm-apps/out-prun-spec/

read -a config_arr <<<$CONFIG
TITLE="Benchmark, "
DEGRA=""
config_count=0
for config in $CONFIG; do
    TITLE="${TITLE}${config}, "
    DEGRA="${DEGRA}${config}-degradation, "
    config_count=$(expr $config_count + 1)
    #(( config_count++ ))
done
config_count=$(expr $config_count - 1)
#echo "Benchmark," "Metaallo-Dangling," "Baseline," "safestack," "Dsan Degradation," "Safestack Degradation"
echo "${TITLE}${DEGRA}"
echo "Count $config_count"
#exit

declare -a configoutput 
declare -a configTotal
for bench in $BENCHMARKS; do
    for config in `seq 0 "$config_count"`; do
        configoutput[${config}]=(`grep -i "$bench" "${config_arr[${config}]}/1-times.txt" | awk -F '=' '{print $2}'`)
        configTotal[${config}]=0
    done
    #metaoutput=(`grep -i "$bench" "metaalloc-var-8b-iter${ITER}/1-times.txt" | awk -F '=' '{print $2}'`)
    #baseoutput=(`grep -i "$bench" "baseline-iter${ITER}/1-times.txt" | awk -F '=' '{print $2}'`) 
    #safeoutput=(`grep -i "$bench" "baselinesafestack-iter${ITER}/1-times.txt" | awk -F '=' '{print $2}'`)
    #metaTotal=0
    #baseTotal=0
    #safeTotal=0
    for i in `seq 1 "$PRUNITER"`; do
        printoutput="$bench, "
        for config in `seq 0 "$config_count"`; do
            output=${configoutput[${config}]}
            printoutput="${printoutput}${output[i-1]}, "
        done
        #echo "$bench, ${metaoutput[i-1]}, ${baseoutput[i-1]}";
        echo $printoutput
        metaTotal=(`echo "scale=5;$metaTotal  + ${metaoutput[i-1]}" | bc -l`)
        baseTotal=(`echo "scale=5;$baseTotal  + ${baseoutput[i-1]}" | bc -l`)
        safeTotal=(`echo "scale=5;$safeTotal  + ${safeoutput[i-1]}" | bc -l`)
    done
    metapercent=(`echo "scale=3;$metaTotal / $baseTotal" | bc -l`)
    safepercent=(`echo "scale=3;$safeTotal / $baseTotal" | bc -l`)
    echo "TOTAL:$bench, $metaTotal, $baseTotal, $safeTotal, $metapercent, $safeTotal"
    echo -e "\n"
done
