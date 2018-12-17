sudo apt-get update && apt-get upgrade
apt-get install default-jdk
sudo wget https://github.com/protocolbuffers/protobuf/releases/download/v2.5.0/protobuf-2.5.0.tar.gz
sudo mkdir -p /usr/local/proto
sudo tar xf protobuf-2.5.0.tar.gz 
sudo ./protobuf-2.5.0/configure --prefix=/usr/local/proto
sudo make
sudo make check
sudo make install
export PKG_CONFIG_PATH=/usr/local/proto/lib/pkgconfig/
echo PKG_CONFIG_PATH=/usr/local/proto/lib/pkgconfig/ >> ~/.bashrc
export PATH=/usr/local/proto/bin:$PATH
echo PATH=/usr/local/proto/bin:$PATH  >> ~/.bashrc

sudo apt-get install cmake
sudo wget https://archive.apache.org/dist/maven/maven-3/3.3.9/binaries/apache-maven-3.3.9-bin.tar.gz
sudo tar xf apache-maven-3.3.9-bin.tar.gz 
export M2_HOME=/home/ahmad/apache-maven-3.3.9
echo M2_HOME=/home/ahmad/apache-maven-3.3.9  >> ~/.bashrc
export PATH=$M2_HOME/bin:$PATH
echo PATH=$M2_HOME/bin:$PATH >> ~/.bashrc


cd hadoop-3.1.0-src

#as root
  mvn clean install -DskipTests=true
  mvn package -Pdist,native -DskipTests=true -Dtar

  tar xf hadoop-project-dist/target/hadoop-project-dist-3.1.0.tar.gz
  mv hadoop-project-dist/target/hadoop-project-dist-3.1.0 -T /usr/local/hadoop1
#set java home for root user


apt-get install zlib1g-dev
apt-get install pkg-config

#remove target files 
sudo apt-get install libssl-dev
#doop-dist/target/


