#!/bin/bash
cd /var/lib/ceph
cd `ls -d */ | head -n1`

echo "...revert unit.run.bak to unit.run"
directories=$(ls -d */)
for d in ${directories}; do 
    RUN_FILE="${d}unit.run"
    BAK_FILE="${d}unit.run.bak"
    if [[ -e ${BAK_FILE} ]]; then 
        rm ${RUN_FILE}
        mv ${BAK_FILE} ${RUN_FILE}
        echo "Done revert: ${RUN_FILE}"
    fi 
done

