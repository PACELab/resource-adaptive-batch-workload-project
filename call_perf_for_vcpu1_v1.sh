#!/bin/bash

sh -c 'echo -1 >/proc/sys/kernel/perf_event_paranoid'

vmName=${1}

pid2=$(cat /sys/fs/cgroup/cpuset/machine/${vmName}.libvirt-qemu/vcpu1/tasks)
perf stat --sync -e  cycles,instructions -p ${pid2} sleep 1 &> /tmp/${vmName}_vcpu1_perf.log
