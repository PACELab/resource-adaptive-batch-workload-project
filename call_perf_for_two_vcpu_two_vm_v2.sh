#!/bin/bash


vmName=${1}
vmName2=${2}

#pid1=$(cat /sys/fs/cgroup/cpuset/machine/${vmName}.libvirt-qemu/vcpu0/tasks)
#pid2=$(cat /sys/fs/cgroup/cpuset/machine/${vmName}.libvirt-qemu/vcpu1/tasks)

#echo ${pid1}" "${pid2}

#perf stat --sync -e  cycles,instructions -p ${pid1} sleep 1 &> /tmp/${vmName}_vcpu0_perf.log & 
#perf stat --sync -e  cycles,instructions -p ${pid2} sleep 1 &> /tmp/${vmName}_vcpu1_perf.log &

/home/ahmad/spark-intereference-project/call_perf_for_given_vcpu.sh ${vmName} 0 &
/home/ahmad/spark-intereference-project/call_perf_for_given_vcpu.sh ${vmName} 1 &
/home/ahmad/spark-intereference-project/call_perf_for_given_vcpu.sh ${vmName2} 0 &
/home/ahmad/spark-intereference-project/call_perf_for_given_vcpu.sh ${vmName2} 1 &

wait 

