#!/bin/bash


vmName=${1}

/home/ahmad/spark-intereference-project/call_perf_for_given_vcpu.sh ${vmName} 0

ipc0=$(cat /tmp/${vmName}_vcpu0_perf.log | grep instructions | awk '{print $4}')

#echo ${ipc0}

if [ -z "${ipc0}" ]
then
     ipc0="0.0"	 
fi

digist0=$(echo "${ipc0//.}")


if [[ ${digist0} =~ ^[0-9]+$ ]]
then
   	meanIPC=${ipc0}
else
    	meanIPC="0.0"
fi

echo ${meanIPC}

