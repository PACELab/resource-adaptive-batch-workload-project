docker exec -d --user test $1 /usr/bin/stress-ng -vm 1 --vm-bytes $2

echo "docker exec -d --user test $1 /usr/bin/stress-ng -vm 1 --vm-bytes $2"
