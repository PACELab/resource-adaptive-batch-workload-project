#!/bin/bash

sh -c 'echo -1 >/proc/sys/kernel/perf_event_paranoid'

vmName=${1}

#pid1=$(cat /sys/fs/cgroup/cpuset/machine/${vmName}.libvirt-qemu/vcpu0/tasks)
#pid2=$(cat /sys/fs/cgroup/cpuset/machine/${vmName}.libvirt-qemu/vcpu1/tasks)

#echo ${pid1}" "${pid2}

#perf stat --sync -e  cycles,instructions -p ${pid1} sleep 1 &> /tmp/${vmName}_vcpu0_perf.log & 
#perf stat --sync -e  cycles,instructions -p ${pid2} sleep 1 &> /tmp/${vmName}_vcpu1_perf.log &

/home/ahmad/spark-intereference-project/call_perf_for_vcpu0_v1.sh ${vmName} &
/home/ahmad/spark-intereference-project/call_perf_for_vcpu1_v1.sh ${vmName} &

wait 

#vcpu0=$(perf stat --sync -e  cycles,instructions -p ${pid1} sleep 1) &
#vcpu1=$(perf stat --sync -e  cycles,instructions -p ${pid2} sleep 1)
#sleep 0.01 


#ipc0=$(echo ${vcpu0} | cut -f2 -d '#')
#ipc1=$(echo ${vcpu1} | cut -f2 -d '#')

#echo ${ipc0}
#echo ${ipc1}

ipc0=$(cat /tmp/${vmName}_vcpu0_perf.log | grep instructions | awk '{print $4}')
ipc1=$(cat /tmp/${vmName}_vcpu1_perf.log | grep instructions | awk '{print $4}')

if [ -z "${ipc0}" ]
then
      #echo "ipc0 is empty" 
      ipc0=${ipc1}
fi

if [ -z "${ipc1}" ]
then
      #echo "ipc1 is empty"
      ipc1=${ipc0}
fi

#echo ${ipc0}" "${ipc1}

total=$(echo "${ipc0} + ${ipc1}" | bc)
#echo ${total}

meanIPC=$(echo "${total}/2" | bc -l) 

echo ${meanIPC}
