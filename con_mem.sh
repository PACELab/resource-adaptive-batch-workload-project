while true;do
	cat /sys/fs/cgroup/memory/docker/cbc0788f0dc42d4526fe151c093d702efac5b483fba51a104971662908dee17b/memory.usage_in_bytes >> cbc0788f0dc4.txt
        sleep 1        
done
