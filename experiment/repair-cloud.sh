#!/bin/bash

set -e

if [[ $1 == "-h" ]] || [[ $# -eq 0 ]]; then
    echo "--- Test cloud (16+2) ceph osd repair ---"
    echo "./repair-cloud.sh stripe_unit(K) jerasure/clay/myclay osd_idx"
    echo "  osd_idx is index of PG map. NOT be 0 (Primary)"
    echo "  osd_idx may be one(1-fail) or two(2-fail)"
    echo "eg ./repair-cloud.sh 2048 jerasure 2 6"
    exit 0
fi

STRIPE_UNIT=$(($1*1024))
MODE=$2
OSD_IDX=$3
if [[ -n $4 ]]; then 
    OSD_IDX2=$4
fi
if [[ $OSD_IDX -eq 0 ]]; then echo "xx OSD NOT be Primary"; exit 1; fi 
if [[ -n $OSD_IDX2 ]] && [[ $OSD_IDX2 -eq 0 ]]; then echo "xx OSD NOT be Primary"; exit 1; fi 
CEPH_ID=$(cat /etc/ceph/ceph.conf |grep fsid|cut -d' ' -f3)
echo ">> CEPH_ID: $CEPH_ID"

# clean journal
echo ">> cleaning journalctl log..."
for i in {1..18}; do 
    REMOTE=`printf "machine%03d" $i`
    sshpass -p "Cloud2023" ssh root@$REMOTE "journalctl --rotate  && journalctl --vacuum-size=1K &>/dev/null"
done

# create pool
echo "ceph osd pool rm claypool claypool --yes-i-really-really-mean-it" > help.sh
if [[ $MODE == "jerasure" ]]; then
    echo "ceph osd erasure-code-profile set clay_16_2 plugin=jerasure technique=reed_sol_van k=16 m=2 crush-failure-domain=osd stripe_unit=$STRIPE_UNIT --force" >> help.sh
elif [[ $MODE == "clay" ]]; then 
    echo "ceph osd erasure-code-profile set clay_16_2 plugin=myclay scalar_mds=jerasure technique=reed_sol_van k=16 m=2 d=17 crush-failure-domain=osd stripe_unit=$STRIPE_UNIT mymode=clay io_thre=256 --force" >> help.sh
elif [[ $MODE == "myclay" ]]; then 
    echo "ceph osd erasure-code-profile set clay_16_2 plugin=myclay scalar_mds=jerasure technique=reed_sol_van k=16 m=2 d=17 crush-failure-domain=osd stripe_unit=$STRIPE_UNIT mymode=myclay io_thre=256 --force" >> help.sh
else 
    echo "xx MODE wrong"
    exit 2
fi
echo "ceph osd pool create claypool 1 1 erasure clay_16_2" >> help.sh
echo "ceph osd pool set claypool min_size 16" >> help.sh 
sudo cephadm shell -m help.sh:/help.sh -- bash help.sh
echo ">> sleep 3..." && sleep 3

# parse PG
PGMAP=`sudo cephadm shell -- ceph pg dump pgs_brief | awk 'NR>1 && length($3)>12 {print $3}'`
if [[ -z $PGMAP ]]; then
    echo ">> PGMAP empty. wait..." && sleep 3
    PGMAP=`sudo cephadm shell -- ceph pg dump pgs_brief | awk 'NR>1 && length($3)>12 {print $3}'`
fi 
PGMAP=${PGMAP:1:${#PGMAP}-2}
OSD=`echo $PGMAP | cut -d, -f$(($OSD_IDX+1))`
if [[ -n $OSD_IDX2 ]]; then OSD2=`echo $PGMAP | cut -d, -f$(($OSD_IDX2+1))`; fi
echo ">> PGMAP: $PGMAP, IDX-$OSD_IDX -> OSD-$OSD"
if [[ -n $OSD_IDX2 ]]; then echo "IDX-$OSD_IDX2 -> OSD-$OSD2"; fi

# stop osds
HOST=`sudo cephadm shell -- ceph osd find $OSD | grep -m1 -Po '"host": "\K[^"]+'`
echo ">> stopping osd.$OSD on host.$HOST"
sshpass -p Cloud2023 ssh root@$HOST "systemctl stop ceph-$CEPH_ID@osd.$OSD"
if [[ -n $OSD_IDX2 ]]; then 
    HOST2=`sudo cephadm shell -- ceph osd find $OSD2 | grep -m1 -Po '"host": "\K[^"]+'`
    echo ">> stopping osd.$OSD2 on host.$HOST2"
    sshpass -p Cloud2023 ssh root@$HOST2 "systemctl stop ceph-$CEPH_ID@osd.$OSD2"
fi 

# write
echo "dd if=/dev/urandom of=infile bs=$STRIPE_UNIT count=16" > help.sh
echo 'for i in {0..999}; do rados -p claypool put NYAN$i infile && echo $i; done' >> help.sh 
sudo cephadm shell -m help.sh:/help.sh -- bash help.sh
echo ">> sleep 3..." && sleep 3

# start osds
echo ">> starting osd.$OSD on host.$HOST"
sshpass -p Cloud2023 ssh root@$HOST "systemctl start ceph-$CEPH_ID@osd.$OSD"
if [[ -n $OSD_IDX2 ]]; then 
    echo ">> starting osd.$OSD2 on host.$HOST2"
    sshpass -p Cloud2023 ssh root@$HOST2 "systemctl start ceph-$CEPH_ID@osd.$OSD2"
fi 

# wait repair
echo 'PG_NO=`rados lspools | wc -l`' > help.sh
echo 'FINISH="$PG_NO active+clean"' >> help.sh
echo 'PGSTAT=""' >> help.sh
echo 'while ! echo $PGSTAT | grep -q "$FINISH"; do echo "wait for repair..." && sleep 3;  PGSTAT=`ceph pg stat`; done' >> help.sh 
sudo cephadm shell -m help.sh:/help.sh -- bash help.sh

# journal
PRIMARY=`echo $PGMAP | cut -d, -f1`
PRIMARY_HOST=`sudo cephadm shell -- ceph osd find $PRIMARY | grep -m1 -Po '"host": "\K[^"]+'`
echo ">> getting log from osd.$PRIMARY on host.$PRIMARY_HOST"
if [[ -n $OSD_IDX2 ]]; then 
    SUFFIX=2
else 
    SUFFIX=1
fi
sshpass -p Cloud2023 ssh root@$PRIMARY_HOST "journalctl -u ceph-$CEPH_ID@osd.$PRIMARY | grep ErasureCodeMyClay > $MODE$SUFFIX.log"
sshpass -p Cloud2023 scp root@$PRIMARY_HOST:$MODE$SUFFIX.log . 