#! /usr/bin/python

import sys,commands
import pandas as pd
import matplotlib.pyplot as plt 

ipFile = sys.argv[1]
traceFilename = sys.argv[2]
#simresults_phCh%d_stdDev%.2f.txt
brokenName = ipFile.split('_')
print "\t [0]: %s [1]: %s [2]: %s \n"%(brokenName[0],brokenName[1],brokenName[2])
phCh = int(brokenName[2].split('phCh')[1])
stdDev = float(brokenName[3].split('stdDev')[1].split('.txt')[0])
print "\t phCh: %d stdDev: %.2f"%(phCh,stdDev)

cols = ["fgBW","bgBW","upperLimit","lowerLimit"]
data = pd.read_csv(ipFile,sep=",",header=None)
data.columns = cols

print "\t Head: %s "%(data.head())
xaxisPts = [i for i in range(len(data))]
plt.figure(figsize=(10,5))
legendArr = []
for curColumn in data.columns:
    plt.plot(xaxisPts,data[curColumn].tolist(),linewidth=2)
    legendArr.append(curColumn)

plt.legend(legendArr,loc='best')
plt.xlabel('time(s)')
plt.ylabel('Bandwidth (Mbps')
titleStr = "Trace: May12, phase-change: "+str(phCh)+" std-dev: "+str(stdDev)
outFilename = "simtrace/plots/"+str(traceFilename)+"_phCh"+str(phCh)+"_stdDev"+str(stdDev)+".jpg"
plt.title(titleStr)
plt.tight_layout()
plt.savefig(outFilename,format='jpg')



