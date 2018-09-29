#! /usr/bin/python

import sys,commands
import pandas as pd 
import matplotlib.pyplot as plt 

numIters = 1
numSecsPerIter = 350
initBWThreshold = 70

def parseFile(inFile):
    perlCmd = "perl -i -pe 's/\|/\t/g' "+str(inFile)
    commands.getoutput(perlCmd)

    txFile = 'tx_'+str(inFile)
    extCmd = "awk '{print $10}' "+str(inFile)+" | perl -pe 's/M//g' > "+str(txFile)
    commands.getoutput(extCmd)

    kTransformCmd = "awk '{if(substr($1,5,1)==\"k\") print substr($1,1,4)/(1024); else print }' "+str(txFile)+" > meh.log"
    print "\t kTransformCmd: %s "%(kTransformCmd)
    commands.getoutput(kTransformCmd)

    kTransformCmd = "awk '{if(substr($1,4,1)==\"k\") print substr($1,1,3)/(1024); else print }' meh.log > "+str(txFile)
    commands.getoutput(kTransformCmd)

    kTransformCmd = "awk '{if(substr($1,3,1)==\"k\") print substr($1,1,2)/(1024); else print }' "+str(txFile)+" > meh.log"
    commands.getoutput(kTransformCmd)

    kTransformCmd = "awk '{if(substr($1,2,1)==\"k\") print substr($1,1,1)/(1024); else print }' meh.log > "+str(txFile)
    commands.getoutput(kTransformCmd)

    kTransformCmd = "awk '{if(substr($1,5,1)==\"B\") print substr($1,1,4)/(1024*1024); else print }' "+str(txFile)+" > meh.log"
    commands.getoutput(kTransformCmd)

    kTransformCmd = "awk '{if(substr($1,4,1)==\"B\") print substr($1,1,3)/(1024*1024); else print }' meh.log > "+str(txFile)
    commands.getoutput(kTransformCmd)

    kTransformCmd = "awk '{if(substr($1,3,1)==\"B\") print substr($1,1,2)/(1024*1024); else print }' "+str(txFile)+" > meh.log"
    commands.getoutput(kTransformCmd)

    kTransformCmd = "awk '{if(substr($1,2,1)==\"B\") print substr($1,1,1)/(1024*1024); else print }' meh.log > "+str(txFile)
    commands.getoutput(kTransformCmd)

    numLinesCmd = "wc -l "+str(txFile)
    numLines = commands.getoutput(numLinesCmd)
    numLines = int(numLines.split(" ")[0].strip())
    sedCmd = " sed -n '5,"+str(numLines)+"p' "+str(txFile)+" > meh.log "
    print "\t sedCmd: %s "%(sedCmd)
    commands.getoutput(sedCmd)
    mvCmd = "mv meh.log "+str(txFile)
    commands.getoutput(mvCmd)

def splitIters(ipData):

    initBW = ipData[ ipData["bwUsed"]>initBWThreshold ]
    initBWIdx = initBW.index.tolist()
    #print "\t initBWIdx-len: %d head: %s \n"%(len(initBWIdx),initBW.head()) 
    curIter=1;
    useBWIdx = initBWIdx[0] 
    useIdx = 0

    startIters = []
    while(curIter<=numIters):
        #print "\t curIter: %d curBWIdx: %d "%(curIter,useBWIdx)
        startIters.append(useBWIdx)
        for curIdx in range(useIdx,len(initBW)):
            curBWIdx = initBWIdx[curIdx]
            #print "\t\t curIdx: %d curBWIdx: %d  "%(curIdx,curBWIdx)
            if(curBWIdx>(useBWIdx+numSecsPerIter)):
                useBWIdx = curBWIdx
                useIdx = curIdx
                #print "\t **** curIdx: %d curBWIdx: %d ****"%(curIdx,curBWIdx)
                break;
        curIter+=1

    splicedData = []
    for curIterStart in startIters:
        curSet = ipData["bwUsed"].ix[curIterStart:curIterStart+numSecsPerIter]
        for curEle in curSet:
            splicedData.append(curEle)
    return splicedData

withoutBgFile = sys.argv[1]
withBgFile = sys.argv[2]
titleSuffix = sys.argv[3]

print "\t withoutBgFile: %s withBgFile: %s "%(withoutBgFile,withBgFile)
parseFile(withBgFile)
parseFile(withoutBgFile)

txWithoutBgFile = 'tx_'+str(withoutBgFile)
txWithBgFile = 'tx_'+str(withBgFile)

txWithBg = pd.read_csv(txWithBgFile,sep=" ",header=None); txWithBg.columns = ["bwUsed"]
txWithoutBg = pd.read_csv(txWithoutBgFile,sep=" ",header=None); txWithoutBg.columns = ["bwUsed"]

toUseLen = 0
#withLen = len(txWithBg); withoutLen = len(txWithoutBg)
#toUseLen = withLen if (withLen<withoutLen) else withoutLen
#print "\t toUseLen: %d withLen: %d "%(toUseLen,withLen)

splitPerIter_withBg = splitIters(txWithBg)
splitPerIter_withoutBg = splitIters(txWithoutBg) 

print "\t splitPerIter_withBg: %s "%(len(splitPerIter_withBg))
print "\t splitPerIter_withoutBg: %s "%(len(splitPerIter_withoutBg))
toUseLen = len(splitPerIter_withBg)
xaxis = [i for i in range(1,toUseLen+1)]
diff = [ (a-b) for (a,b) in zip(splitPerIter_withoutBg,splitPerIter_withBg)]
yaxis = [splitPerIter_withBg,splitPerIter_withoutBg,diff]
legendArr = ["withBg","withoutBg","diff"];#legendArr = ["diff"]
#legendArr = ["withBg","diff"];#legendArr = ["diff"]

plt.figure(figsize=(12,6))
plt.plot(xaxis,yaxis[0],color='red',linewidth=2)
plt.plot(xaxis,yaxis[1],color='black',linewidth=2)
plt.plot(xaxis,yaxis[2],color='green',linewidth=2)
plt.legend(legendArr,loc='best',fontsize='9')


plt.xlabel('time(s)')
plt.ylabel('used bandwidth(MB)')
plt.title('Comparing bandwidth obtained with params '+str(titleSuffix),fontsize='10')

filePrefix = withBgFile.split('.log')[0]
saveFileName = 'PLT_'+str(filePrefix)+'.jpg'
plt.tight_layout()
plt.savefig(saveFileName,format='jpg')

