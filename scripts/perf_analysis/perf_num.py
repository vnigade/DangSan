import os
import subprocess
from collections import defaultdict

CONFIG=os.environ['CONFIG']
BENCHMARKS=os.environ['BENCHMARKS'].split()
PRUNITER=4
ITER=1

RESULT_PATH="/home/vne500/Metalloc/autosetup.dir/llvm-apps/out-prun-spec/"
os.chdir(RESULT_PATH)

CONFIG=CONFIG.split(" ")
CONFIG=filter(None, CONFIG)
TITLE="Benchmark," + ",".join(str(item) for item in CONFIG)
for config in CONFIG:
    TITLE=TITLE + ",%s-degradation" % (config)
print TITLE

for benchmark in BENCHMARKS:
    configoutput = dict()
    configtotal = defaultdict(list) 
    for config in CONFIG:
        cmd = "grep -i {0} {1}/1-times.txt | awk -F '=' '{{print $2}}'".format(benchmark, config)
        #print cmd
        configoutput[config] = subprocess.check_output(cmd, shell=True)
        #print "%s %s" % (config, benchmark)
        configoutput[config] = configoutput[config].split()
        
        #print "Configoutput %s" % (configoutput[config])
        total = 0.0 
        for output in configoutput[config]:
            total = total + float(output)
        configtotal[config] = total
        #print "Total %f" % (configtotal[config])
      
    # Print Benchmarks 
    for i in range(0, PRUNITER):
        printoutput = "%s" % (benchmark)
        for config in CONFIG:
            printoutput = printoutput + ",%s" % (configoutput[config][i])
        print printoutput
    
    # Print Total
    printTotal = "Total:%s" % (benchmark)
    printDegra = ""
    for config in CONFIG:
        printTotal = printTotal + ",%s" % (configtotal[config])
        printDegra = printDegra + ",%s" % round((configtotal[config] / configtotal[CONFIG[0]]), 3)
    print printTotal + printDegra + "\n"
