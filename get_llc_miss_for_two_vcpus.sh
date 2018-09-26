#!/bin/bash

vmName=${1}


cacheMiss0=$(cat /tmp/${vmName}_vcpu0_perf.log | grep LLC-load-misses | awk '{print $4}' | cut -f1 -d '%')
cacheMiss1=$(cat /tmp/${vmName}_vcpu1_perf.log | grep LLC-load-misses | awk '{print $4}' | cut -f1 -d '%')

#echo ${cacheMiss0}
#echo ${cacheMiss1}

if [ -z "${cacheMiss0}" ]
then
      #echo "cacheMiss0 is empty" 
      cacheMiss0=${cacheMiss1}
fi

if [ -z "${cacheMiss1}" ]
then
      #echo "cacheMiss1 is empty"
      cacheMiss1=${cacheMiss0}
fi

if [ -z "${cacheMiss0}" ]
then
      if [ -z "${cacheMiss1}" ]
	then
      		cacheMiss0="0.0"
     	 	cacheMiss1="0.0"
	fi

fi
digist0=$(echo "${cacheMiss0//.}")

#echo ${digist0}

digist1=$(echo "${cacheMiss1//.}")
#echo ${digist1}

if [[ ${digist0} =~ ^[0-9]+$ ]]
then
        if [[ ${digist1} =~ ^[0-9]+$ ]]
        then
                total=$(echo "${cacheMiss0} + ${cacheMiss1}" | bc)
                meanCacheMiss=$(echo "${total}/2" | bc -l)

        else
                meanCacheMiss=${cacheMiss0}
        fi
else
        if [[ ${digist1} =~ ^[0-9]+$ ]]
        then
                meanCacheMiss=${cacheMiss1}
        else
                meanCacheMiss="0.0"
        fi
fi

echo ${meanCacheMiss}



