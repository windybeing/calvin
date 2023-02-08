print() {
for line in {37..78};
do 
    arg=$line"p"
    sed -n $arg ./output.txt
done
}

./copy_binary.sh
cat /dev/null > tpcc_test_result.txt
for contention in 1 5 10 20;
# for contention in 0 0.99 ;
do
    for i in {1};
    do
        bin/deployment/cluster -c deploy-run.conf -p src/deployment/portfile -d bin/deployment/db t $contention 0 0 | tee output.txt &
        sleep 120
        pkill cluster
        sleep 10
        echo -n 'remote param='$contention' tput: ' >> tpcc_test_result.txt
        print | awk '{s += $3} END {print s/6}' | tee -a tpcc_test_result.txt
    done
done

