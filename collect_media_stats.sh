#!/bin/bash

rm -rf /home/ubuntu/media-streaming/output/bgc*

for i in 1 2 3 4 5
do
    echo "Starting run $i ... Start Bg process within 10secs"
    sleep 15
    ./run_media_streaming_v1.sh
    mkdir /home/ubuntu/media-streaming/output/bgc$i
    mv /home/ubuntu/media-streaming/output/result*.log /home/ubuntu/media-streaming/output/bgc$i/.
done

awk '/Overall reply rate/ {print $4}' /home/ubuntu/media-streaming/output/bgc*/result*.log | awk '{s+=$1} END {print "Overall reply rate -> " s/20}'
awk '/Reply time/ {print $5, $7}' /home/ubuntu/media-streaming/output/bgc*/result*.log | awk '{s+=$1; t+=$2} END {print "Reply Rate Response -> " s/20; print "Reply rate Transfer -> " t/20}'
awk '/Errors: total/ {print $3}' /home/ubuntu/media-streaming/output/bgc*/result*.log | awk '{c+=$1} END {print "Timeouts ->" c/20}'
