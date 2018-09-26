#!/bin/bash

sh -c 'echo -1 >/proc/sys/kernel/perf_event_paranoid'

vmName=${1}

pid1=$(cat /sys/fs/cgroup/cpuset/machine/${vmName}.libvirt-qemu/vcpu0/tasks)

perf stat --sync -e  cycles,instructions -p ${pid1} sleep 1 &> /tmp/${vmName}_vcpu0_perf.log 
