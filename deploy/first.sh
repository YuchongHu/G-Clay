#!/bin/bash
set -e
PASSWORD="Cloud2023"

sudo apt update
sudo apt install sshpass -y

# /etc/hosts
if ! grep -q "machine018" /etc/hosts; then
    printf "\n" >> /etc/hosts
    for i in {1..18}; do
        printf "192.168.5.%d\tmachine%03d\tmachine%03d\n" $(($i+41)) $i $i >> /etc/hosts
    done
fi

# cephfiles
cp -r cephfiles /
rm -r cephfiles 
cp /cephfiles/libstdc++.so.6.0.29 /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.25

# install env for all hosts
for i in {1..18}; do 
    REMOTE=`printf "machine%03d" $i`
    sshpass -p $PASSWORD scp -o StrictHostKeyChecking=no env.sh cephadm root@$REMOTE:/root
    sshpass -p $PASSWORD scp -r /cephfiles root@$REMOTE:/
    sshpass -p $PASSWORD ssh root@$REMOTE ./env.sh &
done

# check install done
for i in {1..18}; do 
    REMOTE=`printf "machine%03d" $i`
    INSTALL_DONE=0
    while [ $INSTALL_DONE -eq 0 ]; do
        if sshpass -p Cloud2023 ssh root@$REMOTE ntptime &>/dev/null && lvm version &>/dev/null && docker version &>/dev/null; then
            echo "$REMOTE Install done"
            INSTALL_DONE=1
        else 
            echo "wait 5s..." && sleep 5
        fi
    done 
done


sudo cephadm bootstrap --mon-ip 192.168.5.42  # 192.168 / 172.19

for i in {1..18}; do 
    REMOTE=`printf "machine%03d" $i`
    sshpass -p $PASSWORD ssh-copy-id -f -i /etc/ceph/ceph.pub root@$REMOTE
done

