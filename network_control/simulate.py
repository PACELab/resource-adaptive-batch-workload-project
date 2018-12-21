#! /usr/bin/python

import sys,subprocess,commands

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
def getParam(summaryFilename,curParam):
    #print "\t param: %s "%(curParam)
    grepCmd = "grep "+str(curParam)+" "+str(summaryFilename)+" | awk '{print $2}'"
    grepOutput = commands.getoutput(grepCmd).strip() 
    #print "\t grepOutput: %s "%(grepOutput)
    return grepOutput

phaseChangeArr = [3,5,10]
stdDevThreshold = [0.5,1,1.5]
traces = ["may12_trace1"] #.txt is added in network-control
printTrace = 1 # 0: no simtrace, 1: with sim trace 2: with simtrace + debug messages
summarizedOutputs = {}
for curTrace in traces:
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
            print "\t summarizedOutputs[curTrace][curPhaseChWindow][curStdDev]: %s "%(summarizedOutputs[curTrace][curPhaseChWindow][curStdDev])
            break;

summaryFilename = "summary/curSummary.txt"
summaryFile = open(summaryFilename,'w')

for curParam in params:
    summaryFile.write("Parameter: %s \n"%(curParam))
    for curTrace in traces:
        outPrefix = str(curTrace)
        for curPhaseChWindow in phaseChangeArr:
            outPrefix = str(outPrefix)+"\t"+str(curPhaseChWindow)
            for curStdDev in stdDevThreshold:
                outPrefix = str(curTrace)+"\t"+str(curPhaseChWindow)+"\t"+str(curStdDev)
                summaryFile.write(str(outPrefix)+"\t"+str(summarizedOutputs[curTrace][curPhaseChWindow][curStdDev][curParam])+"\n")
                break;
    summaryFile.write("\n\n")

        #res = subprocess.check_output(['./network-control.out', '1', '{}'.format(orig_container_bw),'{}'.format(phase), '0.0', '{}'.format(stdev),'{}'.format(reduce_container_bw), 'netstat'])

