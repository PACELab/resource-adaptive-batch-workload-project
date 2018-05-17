docker exec -d --user test $1 /usr/bin/stress-ng --vm 32 --vm-bytes ${2}K
echo "docker exec -d --user test $1 /usr/bin/stress-ng --vm 32 --vm-bytes ${2}K"
