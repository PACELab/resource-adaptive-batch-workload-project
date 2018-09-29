#!/bin/bash


vmName=${1}

#pid1=$(cat /sys/fs/cgroup/cpuset/machine/${vmName}.libvirt-qemu/vcpu0/tasks)
#pid2=$(cat /sys/fs/cgroup/cpuset/machine/${vmName}.libvirt-qemu/vcpu1/tasks)

#echo ${pid1}" "${pid2}

#perf stat --sync -e  cycles,instructions -p ${pid1} sleep 1 &> /tmp/${vmName}_vcpu0_perf.log & 
#perf stat --sync -e  cycles,instructions -p ${pid2} sleep 1 &> /tmp/${vmName}_vcpu1_perf.log &

/home/ahmad/spark-intereference-project/call_perf_for_given_vcpu.sh ${vmName} 0 &
/home/ahmad/spark-intereference-project/call_perf_for_given_vcpu.sh ${vmName} 1 &

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


#echo ${ipc0}
#echo ${ipc1}

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

if [ -z "${ipc0}" ]
then
     if [ -z "${ipc1}" ]
     then
         ipc0="0.0"
	 ipc1="0.0"
     fi
fi

digist0=$(echo "${ipc0//.}")

#echo ${digist0}

digist1=$(echo "${ipc1//.}")
#echo ${digist1}

if [[ ${digist0} =~ ^[0-9]+$ ]]
then
   	if [[ ${digist1} =~ ^[0-9]+$ ]]
	then
		total=$(echo "${ipc0} + ${ipc1}" | bc)
		meanIPC=$(echo "${total}/2" | bc -l)

	else
   		meanIPC=${ipc0}
	fi
else
    	if [[ ${digist1} =~ ^[0-9]+$ ]]
        then
		meanIPC=${ipc1}
        else
        	meanIPC="0.0"
	fi
fi

echo ${meanIPC}

