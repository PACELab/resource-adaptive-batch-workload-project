#! /usr/bin/python
import sys,commands,datetime
""" This file parses the raw tracepktdump output to outputs
the sum of ingress + outgress traffic along with the seconds"""

# TODO: Take input file, output file, marker as cmdline args

# Dump the tracepktdump output to a file and
# use that as an input to this script

file_name = sys.argv[1]
numSpces = int(sys.argv[4])

# TODO: change this value according to the bundle you use
glueStr = ''
for curIdx in range(numSpces):
    glueStr = str(glueStr)+' '

kwd = str(sys.argv[2])+str(glueStr)+str(sys.argv[3])
pkwd = str(sys.argv[2])+str(sys.argv[3])


fp1name = str(pkwd).lower()+'_trace1'+'.txt'
fp2name = str(pkwd).lower()+'_trace2'+'.txt'
fp1 = open(fp1name, 'w')
fp2 = open(fp2name, 'w')

print "\t fp1: %s fp2: %s glueStr: --%s--"%(fp1name,fp2name,glueStr)
print "\t kwd: %s pkwd: %s "%(kwd,pkwd)

res = {}
packet_cnt = {}
lineCnt = 0
prevTime = datetime.datetime.now()
with open(file_name) as fp:
    for line in fp:
        try:
            if kwd in line:
                splitLine = line.split(' ')
                time = line.split(' ')[2+numSpces]
                #print "\t line: --%s-- splitLine: %s time: %s "%(line,splitLine,time)
                hour, min, sec = list(map(int, time.split(":")))
                key = hour * 3600 + min * 60 + sec
                if key in packet_cnt:
                    packet_cnt[key] += 1
                else:
                    packet_cnt[key] = 1
                #print "\t *** key: %s *** "%(key)
            if 'Packet Length:' in line:
                val = int(line.split(' ')[4].split('/')[1])

            if 'Direction Value:' in line:
                index = int(line.split(' ')[-1])

                if key in res:
                    res[key][index] += val
                else:
                    res[key] = [0, 0, 0]
                    res[key][index] += val
        except Exception as e:
            #continue;
            print line, e
            sys.exit()
        
        lineCnt+=1
        if(lineCnt%(1000*1000)==0): 
            currTime = datetime.datetime.now()
            delta = currTime - prevTime
            print "\t line: %d M time: %s "%((lineCnt/1e6),delta)
            #break;
        #if(lineCnt>170*1000*1000): break;
#print res
# print 'Packets/sec: ', packet_cnt


_keys = res.keys()
_keys.sort()

# default output of this script goes to this file
for key in _keys:
    #fp.write('{} \n'.format((res[key][0] + res[key][1]) * 11.8))
    l1 = '%s\n'%(res[key][0]*11.8)
    l2 = '%s\n'%(res[key][1]*11.8)
    fp1.write(l1)
    fp2.write(l2)

