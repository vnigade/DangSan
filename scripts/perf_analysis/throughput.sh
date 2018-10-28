#!/bin/bash

set -e
: ${ITER:=11}

# Compile, build and run
make clean
./configure.llvm
./relink.llvm
./build.llvm

# Start server
./serverctl start

# Start Client run
totalReq=0
for ((i=1;i<=${ITER};i++));
do
#for i in {1..${ITER}}; do
    CLIENT_CP=1 BENCH_TYPE=1 ./clientctl bench
    mv runbench.ini runbench${i}.ini;
    req=(`grep "requests_per_sec" runbench${i}.ini  | awk -F'=' '{print $2}'`)
    totalReq=(`echo "scale=5;$totalReq + $req" | bc -l`)
done

# Stop server 
./serverctl stop
make clean

echo "Total is $totalReq"
avgTotal=(`echo "scale=5;$totalReq / $ITER" | bc -l`)
echo "Average is $avgTotal"
