#!/bin/bash

if [ -z "${2}" ]; then
        sleepTime="1"
else
        sleepTime=${2}
fi

stdbuf -oL perf stat --sync -e  instructions,cycles -p ${1} sleep ${sleepTime} &> /tmp/${1}_perf.log
