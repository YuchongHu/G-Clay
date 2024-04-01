# Publication
Baijian Ma, Yuchong Hu, Dan Feng, Ray Wu, Kevin Zhang. **Repair I/O Optimization for Clay Codes via Gray-Code Based Sub-Chunk Reorganization in Ceph**. In: Proceedings of the 38th International Conference on Massive Storage Systems and Technology (**MSST 2024**), Santa Clara, CA, USA, June 3-7, 2024

## Contact

Please email to Yuchong Hu (yuchonghu@hust.edu.cn) if you have any questions.

# G-Clay Install and Test

## Generate a desired Gray code

> Tip: below mark "myclay" represents the "GClay" plug-in

```shell
cd gray
g++ -o brgc brgc.cpp && ./brgc	# One BRGC n-ary Gray code
g++ -o main main.cpp && ./main	# Lots of binary reflected gray codes
```

Besides, we could follow this https://distrinet.cs.kuleuven.be/software/sce/ariadne.html  to generate Balanced Gray codes.

Write the generated Gray code to the initialization assignment of `vector<int> order ` of the `ErasureCodeMyClay::init()` function under `G-Clay/src/erasure-code/myclay/`

## Build

We are building based on the original ceph, so we need to download the ceph source code first

```shell
cd ~
git clone -b v15.2.17 --single-branch --recurse-submodules  https://github.com/ceph/ceph.git
cd ceph
sudo ./install-deps.sh 
```

Now we switch to the project directory and add the new G-Clay erasure coding plug-in and make some code replacements

```shell
cd G-Clay
./install-src.sh
```

Compile some ceph components, which may require a lot of memory

```shell
cd ~/ceph
mkdir build
cd build
sudo cmake .. -DCMAKE_BUILD_TYPE=Release

cd ~/ceph/build
sudo make ec_jerasure -j
sudo make ec_myclay -j
sudo make ceph-osd -j

# copy to a /cephfiles
sudo cp lib/libec_jerasure.so lib/libec_myclay.so bin/ceph-osd G-Clay/deploy/cephfiles
sudo cp -r G-Clay/deploy/cephfiles /
```



## Cloud testing

Assume that the names of 18 machines start with *machine001*, and the IPs are consecutive, starting from *192.168.5.42*, and password is *Cloud2023*

```shell
# ssh to first remote machine
# install ceph env, and ceph bootstrap
cd deploy
./first.sh

# log in ceph terminal, add all hosts, add osd
sudo cephadm shell -m first-shell.sh:/first-shell.sh
./first-shell.sh
watch ceph -s

# all stop
for i in {1..18}; do 
    REMOTE=`printf "machine%03d" $i`
    sshpass -p Cloud2023 ssh root@$REMOTE "systemctl stop ceph.target"
done

# Mount compiled files
for i in {1..18}; do 
    REMOTE=`printf "machine%03d" $i`
    sshpass -p Cloud2023 ssh root@$REMOTE "cd /cephfiles && ./change.sh"
done

# all start
for i in {1..18}; do 
    REMOTE=`printf "machine%03d" $i`
    sshpass -p Cloud2023 ssh root@$REMOTE "systemctl start ceph.target"
done
```

set network bandwidth using tc

```shell
for i in {1..18}; do 
    REMOTE=`printf "machine%03d" $i`
    sshpass -p Cloud2023 ssh root@$REMOTE "tc qdisc add dev eth0 root tbf rate 1gbit burst 100mbit latency 400ms"
done
```

Then we test using shell script:

```shell
cd experiment
./repair-cloud-all.sh
./encode-read-cloud.sh
```



## Analyze log

Use `analyze/ana-repair` and `analyze/ana-encode-read` to analyze the result log and obtain the time of different repair stages: **disk reading, network transmission and decoding calculation**

eg.

```shell
# ./ana-repair log1/clay1.log log1/myclay1.log
# result:
--------------- stage 1 ---------------
1.636 - Disk Read of  log1/clay1.log (REPAIR=6)
0.536 - Disk Read of log1/myclay1.log (REPAIR=6)
--------------- stage 2 ---------------
0.056 - Network of    log1/clay1.log (REPAIR=6)
0.053 - Network of   log1/myclay1.log (REPAIR=6)
--------------- stage 3 ---------------
0.060 - Decode of     log1/clay1.log (REPAIR=6)
0.061 - Decode of    log1/myclay1.log (REPAIR=6)
----------------- Sum -----------------
1.752 - Sum of        log1/clay1.log (REPAIR=6)
0.65  - Sum of       log1/myclay1.log (REPAIR=6)
```

