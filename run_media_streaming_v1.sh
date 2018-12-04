#!/bin/bash


find ~/media-streaming/output/ -maxdepth 1 -type f -exec rm -v {} \;

cd /home/ubuntu/media-streaming/client/files/run

startTime=$(date +"%s,%N")

/home/ubuntu/media-streaming/client/files/run/benchmark.sh 130.245.127.187 &> ~/media_streaming.log

endTime=$(date +"%s,%N")

echo "StartTime: ${startTime}" >>  ~/media_streaming.log
echo "EndTime: ${endTime}" >>  ~/media_streaming.log
