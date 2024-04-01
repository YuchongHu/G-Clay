set -e

if [[ $1 == "-h" ]] || [[ $# -eq 0 ]]; then
    echo "--- Test ceph encode and read time ---"
    echo "./encode-read-cloud.sh stripe_unit(K) jerasure/clay/myclay"
    echo "eg ./encode-read-cloud.sh 2048 jerasure"
    exit 0
fi

STRIPE_UNIT=$(($1*1024))
MODE=$2

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
sudo cephadm shell -m help.sh:/help.sh -- bash help.sh
echo ">> sleep 3..." && sleep 3

# parse PG
PGMAP=`sudo cephadm shell -- ceph pg dump pgs_brief | awk 'NR>1 && length($3)>12 {print $3}'`
if [[ -z $PGMAP ]]; then
    echo ">> PGMAP empty. wait..." && sleep 3
    PGMAP=`sudo cephadm shell -- ceph pg dump pgs_brief | awk 'NR>1 && length($3)>12 {print $3}'`
fi 
PGMAP=${PGMAP:1:${#PGMAP}-2}

# write && read
echo "dd if=/dev/urandom of=infile bs=$STRIPE_UNIT count=16" > help.sh  
echo 'for i in {0..999}; do rados -p claypool put NYAN$i infile && echo $i; done' >> help.sh 
echo "sleep 3" >> help.sh 
echo 'for i in {0..999}; do rados -p claypool get NYAN$i outfile && echo read $i; done' >> help.sh
sudo cephadm shell -m help.sh:/help.sh -- bash help.sh

# journal
PRIMARY=`echo $PGMAP | cut -d, -f1`
PRIMARY_HOST=`sudo cephadm shell -- ceph osd find $PRIMARY | grep -m1 -Po '"host": "\K[^"]+'`
echo ">> getting log from osd.$PRIMARY on host.$PRIMARY_HOST"
sshpass -p Cloud2023 ssh root@$PRIMARY_HOST "journalctl -u ceph-$CEPH_ID@osd.$PRIMARY | grep ErasureCodeMyClay > $MODE-encode-read.log"
sshpass -p Cloud2023 scp root@$PRIMARY_HOST:$MODE-encode-read.log .
