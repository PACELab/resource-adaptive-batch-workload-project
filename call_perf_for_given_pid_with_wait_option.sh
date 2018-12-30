#!/bin/bash

#echo  $(date +%s%N)

if [ -z "${2}" ]; then
	perf stat --sync -e  instructions,cycles -p ${1} sleep 1 &> /tmp/${1}_perf.log &
else
	perf stat --sync -e  instructions,cycles -p ${1} sleep 1 &> /tmp/${1}_perf.log
	sleep 0.005
fi

#echo  $(date +%s%N)
