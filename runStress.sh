docker exec -d --user test $1 /usr/bin/stress-ng --vm $2 --vm-bytes ${3}K
echo "docker exec -d --user test $1 /usr/bin/stress-ng --vm $2 --vm-bytes ${3}K"
