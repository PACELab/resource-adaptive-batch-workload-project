docker exec -d --user test $1 /usr/bin/stress-ng -vm $2
echo "updated"
