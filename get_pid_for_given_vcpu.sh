#!/bin/bash

vmName=${1}
vcpu=${2}

pid1=$(cat /sys/fs/cgroup/cpuset/machine/${vmName}.libvirt-qemu/vcpu${vcpu}/tasks)

echo ${pid1}
