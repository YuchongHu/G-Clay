#!/bin/bash
set -e

sudo apt update

# ntp lvm2
echo Y | sudo apt install ntpsec -y 
sudo apt install lvm2 -y

# docker 
sudo apt-get update
sudo apt-get install docker-ce -y

# cephadm 
chmod +x cephadm
sudo cp cephadm /usr/local/bin/
sudo cephadm add-repo --release octopus

# network
sudo apt install iproute2 iperf -y

sudo apt install sshpass -y

