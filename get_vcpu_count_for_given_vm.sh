#!/bin/bash

vmName=${1}
vcpuCount=$(virsh vcpucount ${vmName} | grep current | grep live | awk '{print $3}')

if [ -z "${vcpuCount}" ]
then
     vcpuCount="0"
fi

echo ${vcpuCount} 
