# sudo cephadm shell -m first-shell.sh:/first-shell.sh

for i in {2..18}; do 
    REMOTE=`printf "machine%03d" $i`
    ceph orch host add $REMOTE
done

# ceph orch apply osd --all-available-devices\
for i in {1..18}; do 
    REMOTE=`printf "machine%03d" $i`
    ceph orch daemon add osd $REMOTE:/dev/vdb
done

# ceph settings
ceph osd set noout
ceph config set osd debug_osd 0
ceph config set osd log_to_file false
ceph config set osd osd_recovery_sleep_hdd 0
ceph config set osd osd_recovery_max_chunk 1
ceph config set mon mon_allow_pool_delete true
ceph config set mon osd_pool_default_pg_autoscale_mode off
ceph config set osd osd_max_backfills 300 
ceph config set osd osd_recovery_max_active 300
ceph config set osd osd_recovery_max_single_start 100
ceph config set mon osd_max_write_size 2048
ceph config set osd osd_max_write_size 2048
ceph config set global osd_max_object_size 2147483648