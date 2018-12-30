#!/bin/bash

dir=$(echo $PWD);

sleepTime=${1}
n=${2}
n=$((n+2))
echo $n

for i in $(seq 3 1 ${n})
do
	${dir}/monitor_1_pid_v2.sh &
done
wait

#n=${2}
#n=$((n+2))
#echo $n 

#stdbuf -oL perf stat --sync -e  instructions,cycles -p ${n} sleep ${sleepTime} &> /tmp/${n}_perf.log

#wait 

output=""

for i in $(seq 3 1 ${n})
do
	ipc=$(cat /tmp/${i}_perf.log  |  grep instructions | awk '{print $4}')
	output=${output}" "${ipc}
done

echo ${output}
