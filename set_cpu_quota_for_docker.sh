#!/bin/bash

quota=${1}
echo "${quota}" > /sys/fs/cgroup/cpu,cpuacct/docker/cpu.cfs_quota_us

