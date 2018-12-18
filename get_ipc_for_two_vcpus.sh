#!/bin/bash

vmName=${1}

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

