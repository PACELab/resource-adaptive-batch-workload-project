#!/bin/bash

sh -c 'echo -1 >/proc/sys/kernel/perf_event_paranoid'

vmName=${1}
vcpu=${2}

pid1=$(cat /sys/fs/cgroup/cpuset/machine/${vmName}.libvirt-qemu/vcpu${vcpu}/tasks)

perf stat --sync -e  cache-misses,cache-references,cycles,instructions,LLC-load-misses,LLC-loads -p ${pid1} sleep 1 &> /tmp/${vmName}_vcpu${vcpu}_perf.log 
