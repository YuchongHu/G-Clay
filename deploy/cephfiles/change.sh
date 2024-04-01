#!/bin/bash
cd /var/lib/ceph
cd `ls -d */ | head -n1`

if [[ ! -e /cephfiles/libec_myclay.so ]]; then
    echo "...no libec_myclay.so, unit.run remains same"
    exit 1
fi

echo "...start to modify unit.run: add bind mount"
echo "...original unit.run will be saved as unit.run.bak"
directories=$(ls -d */)
IMAGE=quay.io/ceph/ceph:v15
for d in ${directories}; do 
    RUN_FILE="${d}unit.run"
    if [[ -e ${RUN_FILE} ]] && grep -q $IMAGE ${RUN_FILE}; then 
        if grep -q "libec_myclay.so" ${RUN_FILE}; then  
            echo "Have modified before: ${RUN_FILE}"   
            continue
        fi
        sed -i.bak 's|--rm|& -v /cephfiles/libec_myclay.so:/lib64/ceph/erasure-code/libec_myclay.so -v /cephfiles/libstdc++.so.6.0.29:/lib64/libstdc++.so.6.0.25 -v /cephfiles/ceph-osd:/usr/bin/ceph-osd -v /cephfiles/libec_jerasure.so:/lib64/ceph/erasure-code/libec_jerasure.so|g' ${RUN_FILE}
        echo "Done: ${RUN_FILE}"
    fi 
done

