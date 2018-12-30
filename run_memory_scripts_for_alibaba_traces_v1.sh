#!/bin/bash

list=$(cat /home/ahmad/selected_trace_v1.txt)
for trace in ${list}
do
        echo ${trace}
	#ssh ubuntu@130.245.127.130 "/home/ubuntu/simple_mimic.out /home/ubuntu/alibaba_traces/trace_${trace} &> /dev/null &"& 
        ssh ubuntu@130.245.127.130 "/home/ubuntu/simple_mimic /home/ubuntu/alibaba_traces/trace_${trace} &> /dev/null &"&
        sleep 2
        /home/ahmad/spark-intereference-project/memory_daemon-v6 3 60 60 2 0.05 1.5 1 /usr/local/wrt > /home/ahmad/scavenger_experiments/mem_script_trace${trace}_n100.log &
	pid=$!
	sleep 900
	kill -0 ${pid}
        docker exec e3c693f0e8b2  bash -c "killall stress"
	pkill simple_mimic.out

#	echo ${trace}

done
