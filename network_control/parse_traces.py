""" This file parses the raw tracepktdump output to outputs
the sum of ingress + outgress traffic along with the seconds"""

# TODO: Take input file, output file, marker as cmdline args

# Dump the tracepktdump output to a file and
# use that as an input to this script
file_name = "/data_repo/dl_wits_data/traces.txt"

# TODO: change this value according to the bundle you use
kwd = 'Oct 20'

res = {}
packet_cnt = {}
with open(file_name) as fp:
    for line in fp:
        try:
            if kwd in line:
                time = line.split(' ')[3]
                hour, min, sec = list(map(int, time.split(":")))
                key = hour * 3600 + min * 60 + sec
                if key in packet_cnt:
                    packet_cnt[key] += 1
                else:
                    packet_cnt[key] = 1
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
            print line, e

print res
# print 'Packets/sec: ', packet_cnt


_keys = res.keys()
_keys.sort()

# default output of this script goes to this file
with open('trace_result.txt', 'w') as fp:
    for key in _keys:
        fp.write('{} {} \n'.format(key, res[key][0] + res[key][1]))
