#!/bin/bash

./bargraph.pl -eps ${1}.dat > ${1}.eps
#./fixbb.sh bargraph_runtime_spec.eps
ps2pdf -dEPSCrop ${1}.eps

