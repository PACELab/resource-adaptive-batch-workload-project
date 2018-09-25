#!/bin/bash

#name the containers as per the socket to which they are attached to;
if [ "$#"  -ne 2 ]; then
   echo "usage: ./manage_container <socket_num> <mem_in_gb>"
   exit 1
fi

con_id=$(docker ps | grep socket_$1 | awk '{print $1}')

#before updating the memory of the docker container; be sure to pause it
#this is important because we cannot set a new value that is lower than current memory to the container without pausing the container.

/usr/bin/docker pause $con_id
/usr/bin/docker update -m $2G $con_id
/usr/bin/docker unpause $con_id
#/usr/bin/docker stats $con_id --no-stream
