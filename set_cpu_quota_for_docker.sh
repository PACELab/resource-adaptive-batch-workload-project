#!/bin/bash

quota=${1}

if [ -z "${2}" ]; then
	echo "${quota}" > /sys/fs/cgroup/cpu,cpuacct/docker/cpu.cfs_quota_us
else
        echo "${quota}" > /sys/fs/cgroup/cpu,cpuacct/docker/${2}*/cpu.cfs_quota_us
fi
