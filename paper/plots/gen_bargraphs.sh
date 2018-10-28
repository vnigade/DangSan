#!/bin/bash

./bargraph.pl -eps bargraph_runtime_spec.dat > bargraph_runtime_spec.eps
#./fixbb.sh bargraph_runtime_spec.eps
ps2pdf -dEPSCrop bargraph_runtime_spec.eps

