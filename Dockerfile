FROM ubuntu:trusty
MAINTAINER Jeff Lindsay <progrium@gmail.com>

RUN apt-get update && apt-get install -y stress-ng && apt-get install psmisc
RUN useradd -m -d /test test
