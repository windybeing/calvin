# scp -r bin/ 172.31.29.191:~/calvin/
# scp -r bin 172.31.28.168:~/calvin/
# scp -r paxos 172.31.28.168:~/calvin
# scp ./ext/zookeeper-3.4.6/conf/zoo.cfg 172.31.28.168:~/calvin/ext/zookeeper-3.4.6/conf/zoo.cfg
# ssh -t 172.31.28.168 "echo 2 > /tmp/zookeeper/myid"
# scp -r bin 172.31.23.163:~/calvin/
# scp -r paxos 172.31.23.163:~/calvin
# scp ./ext/zookeeper-3.4.6/conf/zoo.cfg 172.31.23.163:~/calvin/ext/zookeeper-3.4.6/conf/zoo.cfg
# ssh -t 172.31.23.163 "echo 3 > /tmp/zookeeper/myid"

# ssh -t 172.31.29.191 "rm -rf /tmp/zookeeper/version-2"
# ssh -t 172.31.28.168 "rm -rf /tmp/zookeeper/version-2"
# ssh -t 172.31.23.163 "rm -rf /tmp/zookeeper/version-2"

scp -r bin 172.31.28.168:~/calvin
scp -r bin 172.31.23.163:~/calvin
scp -r bin 172.31.21.26:~/calvin
scp -r bin 172.31.23.216:~/calvin
scp -r bin 172.31.30.114:~/calvin
scp -r bin 172.31.20.192:~/calvin
