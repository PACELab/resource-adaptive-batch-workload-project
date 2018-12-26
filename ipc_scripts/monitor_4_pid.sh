#!/bin/bash

perf stat --sync -e  instructions,cycles -p ${1} sleep 1 &> /tmp/${1}_perf.log &
#sleep 0.005
perf stat -e  instructions,cycles -p ${2} sleep 1 &> /tmp/${2}_perf.log &
#sleep 0.005
perf stat -e  instructions,cycles -p ${3} sleep 1 &> /tmp/${3}_perf.log &
#sleep 0.005
perf stat -e  instructions,cycles -p ${4} sleep 1 &> /tmp/${4}_perf.log

wait
