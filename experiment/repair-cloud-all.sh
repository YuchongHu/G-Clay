set -e

if [[ $1 == "-h" ]] || [[ $# -eq 0 ]]; then
    echo "--- Test cloud (16+2) all ceph osd repair ---"
    echo "./repair-cloud-all.sh stripe_unit(K) jerasure/clay/myclay"
    echo "  will test 17 osds repair for jerasure/clay/myclay"
    echo "eg ./repair-cloud-all.sh 2048 jerasure"
    exit 0
fi

STRIPE_UNIT=$1
MODE=$2
OUTFILE="allout${STRIPE_UNIT}K-${MODE}.txt"
if [[ -e $OUTFILE ]]; then rm $OUTFILE; fi

for OSD_IDX in {1..17}; do 
    echo "------------ $MODE $OSD_IDX ------------"
    ./repair-cloud.sh $STRIPE_UNIT $MODE $OSD_IDX
    RESULT=`./ana-repair ${MODE}1.log | tail -1`
    if [[ -z $RESULT ]]; then echo "RESULT empty" && exit 1; fi
    echo "$RESULT" >> $OUTFILE
done 

# clean
rm *.log