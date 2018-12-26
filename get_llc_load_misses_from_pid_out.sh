#!/bin/bash

pid1=${1}

output=$(cat /tmp/${pid1}_perf.log | grep LLC-load-misses | awk '{print $4}' | cut -f1 -d '%')


if [ -z "${output}" ]
then
     ipc0="0"
fi

echo ${output}
