#!/bin/bash

if [ -z "${3}" ]; then
        sleepTime="1"
else
        sleepTime=${3}
fi

dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
#echo ${dir}
#cd ${dir}


stdbuf -oL perf stat --sync -e  instructions,cycles -p ${1} sleep ${sleepTime} &> /tmp/${1}_perf.log &
#sleep 0.005
stdbuf -oL perf stat -e  instructions,cycles -p ${2} sleep ${sleepTime} &> /tmp/${2}_perf.log

wait

#cat /tmp/${1}_perf.log
#cat /tmp/${2}_perf.log

ipc1=$(${dir}/get_ipc_from_pid_out.sh ${1})
ipc2=$(${dir}/get_ipc_from_pid_out.sh ${2})

echo ${ipc1}","${ipc2}

#echo ${ipc1}
#echo ${ipc2}

#sum=$(echo $ipc1+$ipc2 | bc -l)

#echo ${sum}
#printf '%.4f\n' $(echo $sum/2.0 | bc -l) 

#mean=$(echo $sum/2.0 | bc -l)
#echo ${mean}
