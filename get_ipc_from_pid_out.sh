#!/bin/bash

pid1=${1}

ipc0=$(cat /tmp/${pid1}_perf.log | grep instructions | awk '{print $4}')

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
 
