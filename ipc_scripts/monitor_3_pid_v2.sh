#!/bin/bash

if [ -z "${4}" ]; then
        sleepTime="1"
else
        sleepTime=${4}
fi

stdbuf -oL perf stat --sync -e  instructions,cycles -p ${1} sleep ${sleepTime} &> /tmp/${1}_perf.log &
#sleep 0.005
stdbuf -oL perf stat -e  instructions,cycles -p ${2} sleep ${sleepTime} &> /tmp/${2}_perf.log &

stdbuf -oL perf stat -e  instructions,cycles -p ${3} sleep ${sleepTime} &> /tmp/${3}_perf.log

wait

#cat /tmp/${1}_perf.log
#cat /tmp/${2}_perf.log

dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

ipc1=$(${dir}/get_ipc_from_pid_out.sh ${1})
ipc2=$(${dir}/get_ipc_from_pid_out.sh ${2})
ipc3=$(${dir}/get_ipc_from_pid_out.sh ${3})

echo ${ipc1}","${ipc2}","${ipc3}

#echo ${ipc1}
#echo ${ipc2}
#echo ${ipc3}

#sum=$(echo $ipc1+$ipc2 | bc -l)
#sum=$(echo $sum+$ipc3 | bc -l)

#echo ${sum}
#printf '%.4f\n' $(echo $sum/3.0 | bc -l) 

#mean=$(echo $sum/2.0 | bc -l)
#echo ${mean}
