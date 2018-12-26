#!/bin/bash

perf stat --sync -e  instructions,cycles -p ${1} sleep 1 &> /tmp/${1}_perf.log
