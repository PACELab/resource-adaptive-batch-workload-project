import subprocess

orig_container_bw = raw_input('Input Original Container bandwidth (do not specify units): ')
phase_change_samples = raw_input('Input values for phase change samples (comma separated): ').strip().split(',')
std_dev_threshold = raw_input('Input values for std. dev threshold (comma separated): ').strip().split(',')
reduce_container_bw = raw_input('Input the value by which you want to reduce container bandwidth: ')

# Requirements: Get the pktdump of traces, run parse_traces.py on it with appropriate marker
# collect the per second stats, merge multiple files if required for larger duration
# Now manipulate the cpp script to take this file as input and compile the file to rename
# the object file as network-control.out

flag = True
for phase in phase_change_samples:
    for stdev in std_dev_threshold:
        res = subprocess.check_output(['./network-control.out', '1', '{}'.format(orig_container_bw),
                                       '{}'.format(phase), '0.0', '{}'.format(stdev),
                                       '{}'.format(reduce_container_bw), 'netstat'])
        # print res
