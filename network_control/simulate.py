#! /usr/bin/python

import sys,subprocess,commands
import matplotlib.pyplot as plt 
# Requirements: Get the pktdump of traces, run parse_traces.py on it with appropriate marker
# collect the per second stats, merge multiple files if required for larger duration
# Now manipulate the cpp script to take this file as input and compile the file to rename
# the object file as network-control.out

# Plan: 
# 1. Parameters to sweep
#      std-dev threshold; phase-change-samples; traces
# 2. How to sweep
#       traces --> std-dev-threshold --> phase-change-samples.
# 3. Metrics to collect
#   Violations, #Phase changes, FG BW, BG BW, 
# 4. How to summarize
#   per trace, 


#amsuresh@pacews-amsuresh:~/.../scavenger/network_sensitivity_analysis$ cat summary/simresults_may12_trace1_phCh10_stdDev1.50.txt 
#Violations: 951
#PhaseChanges:  12
#FGBW:   172.677
#BGBW:   178.801
#numSamples: 14368

params = ["FGBW","BGBW","Violations","PhaseChanges"]
plotParams = ["BGBW","Violations","PhaseChanges"]
yxais_labels = {
    "FGBW" : "FG BW (Mbps)",
    "BGBW" : "BG BW (Mbps)",
    "Violations" : "Number of violations ",
    "PhaseChanges" : "Number of phase changes ",
}

def getParam(summaryFilename,curParam):
    #print "\t param: %s "%(curParam)
    grepCmd = "grep "+str(curParam)+" "+str(summaryFilename)+" | awk '{print $2}'"
    grepOutput = commands.getoutput(grepCmd).strip() 
    #print "\t grepOutput: %s "%(grepOutput)
    return grepOutput

def plotLine(plotData):
    xaxis = plotData['xaxis']
    xaxis_labels = plotData['xaxis_labels']
    yaxis = plotData['yaxis']
    maxSofar = plotData['maxSofar']
    legendArr = plotData['legend']
    ylabel = plotData['ylabel']
    curParam = plotData['curParam']

    maxSofar = int(float(maxSofar))+1
    plt.figure(figsize=(12,6))
    colors = ['b', 'g', 'r', 'c', 'm', 'y','black','grey','indigo']
    for curIdx in range(len(xaxis)):
        plt.plot(xaxis[curIdx],yaxis[curIdx],linewidth = '1.5',color=colors[curIdx%len(colors)])
    
    plt.legend(legendArr,loc='best')
    plt.xticks(xaxis[0],xaxis_labels[0],rotation='vertical')

    plt.axis([0,len(xaxis[0]),0,maxSofar])
    plt.xlabel('Traces and diff std-dev thresholds')
    plt.ylabel(ylabel)

    titleStr = " Param "+str(curParam)
    plt.title(titleStr)
    filename = "summary/plots/"+str(curParam)+".jpg"
    plt.tight_layout()
    plt.savefig(filename,format="jpg")
    print "\t Title: --%s-- Filename: --%s-- "%(titleStr,filename)

phaseChangeArr = [3,5,10]
stdDevThreshold = [0.5,1,1.5]

traces = [("apr6_trace1","A6"),("apr13_trace1","A13"),("apr20_trace1","A20"),("apr27_trace1","A27"),("may4_trace1","M4")
        ,("may9_trace1","M9"),("may10_trace1","M10"),("may11_trace1","M11"),("may13_trace1","M13"),("may14_trace1","M14")]

printTrace = 1 # 0: no simtrace, 1: with sim trace 2: with simtrace + debug messages
summarizedOutputs = {}
for curTrace,curTraceStr in traces:
    summarizedOutputs[curTrace] = {}
    for curPhaseChWindow in phaseChangeArr:
        summarizedOutputs[curTrace][curPhaseChWindow] = {}
        for curStdDev in stdDevThreshold:
            summarizedOutputs[curTrace][curPhaseChWindow][curStdDev] = {}
            traceRunCmd = "./network-control "+str(curPhaseChWindow)+" "+str(curStdDev)+" 0.01 "+str(printTrace)+" "+str(curTrace)
            print "\t traceRunCmd: %s "%(traceRunCmd)
            #may12_trace1_phCh10_stdDev1.50.txt
            commands.getoutput(traceRunCmd)
            summaryFilename = 'summary/'+str(curTrace)+"_phCh"+str(curPhaseChWindow)+"_stdDev"+str('%.2f'%(curStdDev))+'.txt'
            for curParam in params:
                summarizedOutputs[curTrace][curPhaseChWindow][curStdDev][curParam] = getParam(summaryFilename,curParam)
            #print "\t summarizedOutputs[curTrace][curPhaseChWindow][curStdDev]: %s "%(summarizedOutputs[curTrace][curPhaseChWindow][curStdDev])

summaryFilename = "summary/curSummary.txt"
summaryFile = open(summaryFilename,'w')

for curParam in params:
    summaryFile.write("Parameter: %s \n"%(curParam))
    for curTrace,curTraceStr in traces:
        outPrefix = str(curTrace)
        for curPhaseChWindow in phaseChangeArr:
            outPrefix = str(outPrefix)+"\t"+str(curPhaseChWindow)
            for curStdDev in stdDevThreshold:
                outPrefix = str(curTrace)+"\t"+str(curPhaseChWindow)+"\t"+str(curStdDev)
                summaryFile.write(str(outPrefix)+"\t"+str(summarizedOutputs[curTrace][curPhaseChWindow][curStdDev][curParam])+"\n")

            break;
    summaryFile.write("\n\n")

for curParam in plotParams:
    xaxis =[]; yaxis = []; xaxis_labels = [];legendArr = []
    maxSofar = 0.0
    for curPhaseChWindow in phaseChangeArr:
        curXaxis = []; curXaxisLabels = [] ; curYaxis = []; idx = 0
        for curTrace,curTraceStr in traces:
            for curStdDev in stdDevThreshold:
                outPrefix = str(curTrace)+"\t"+str(curPhaseChWindow)+"\t"+str(curStdDev)
                curXaxisLabels.append(curTraceStr+'_'+str('%.2f'%(curStdDev)))
                curXaxis.append(idx); idx+=1;
                curYaxis.append(summarizedOutputs[curTrace][curPhaseChWindow][curStdDev][curParam])
                if(maxSofar < float(summarizedOutputs[curTrace][curPhaseChWindow][curStdDev][curParam])):
                    maxSofar = float(summarizedOutputs[curTrace][curPhaseChWindow][curStdDev][curParam])
        xaxis.append(curXaxis)
        xaxis_labels.append(curXaxisLabels)
        yaxis.append(curYaxis)
        legendArr.append(curPhaseChWindow)

    plotData = {}
    plotData['xaxis'] = xaxis
    plotData['xaxis_labels'] = xaxis_labels
    plotData['yaxis'] = yaxis
    plotData['maxSofar'] = maxSofar
    plotData['legend'] = legendArr
    plotData['ylabel'] = yxais_labels[curParam]
    plotData['curParam'] = curParam
    plotLine(plotData)

        

