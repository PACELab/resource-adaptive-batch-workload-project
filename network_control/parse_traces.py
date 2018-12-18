#! /usr/bin/python
import sys,commands,datetime
""" This file parses the raw tracepktdump output to outputs
the sum of ingress + outgress traffic along with the seconds"""

# TODO: Take input file, output file, marker as cmdline args

# Dump the tracepktdump output to a file and
# use that as an input to this script
file_name = "trace_files/tracesMay.txt"

# TODO: change this value according to the bundle you use
kwd = 'May 12'
pkwd = 'May12'

res = {}
packet_cnt = {}
lineCnt = 0
prevTime = datetime.datetime.now()
with open(file_name) as fp:
    for line in fp:
        try:
            if kwd in line:
                #print "\t line: %s "%(line)
                time = line.split(' ')[3]
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
        
        lineCnt+=1
        if(lineCnt%(1000*1000)==0): 
            currTime = datetime.datetime.now()
            delta = currTime - prevTime
            print "\t line: %d M time: %s "%((lineCnt/1e6),delta)
            #break;
        #if(lineCnt>100*1000): break;
#print res
# print 'Packets/sec: ', packet_cnt


_keys = res.keys()
_keys.sort()

# default output of this script goes to this file
fp1 = open('trace_result_1_'+str(pkwd)+'.txt', 'w')
fp2 = open('trace_result_2_'+str(pkwd)+'.txt', 'w')
for key in _keys:
    #fp.write('{} \n'.format((res[key][0] + res[key][1]) * 11.8))
    l1 = '%s\n'%(res[key][0]*11.8)
    l2 = '%s\n'%(res[key][1]*11.8)
    fp1.write(l1)
    fp2.write(l2)

