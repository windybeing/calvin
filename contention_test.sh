print() {
for line in {37..90};
do 
    arg=$line"p"
    sed -n $arg ./output.txt
done
}

./copy_binary.sh
cat /dev/null > contention_test_result.txt
for dis_ratio in 0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1;
do
    for contention in 0.1 0.99;
    # for contention in 0 0.99 ;
    do
        for i in 1;
        do
            bin/deployment/cluster -c deploy-run.conf -p src/deployment/portfile -d bin/deployment/db m 0 $contention $dis_ratio | tee output.txt &
            sleep 150
            pkill cluster
            sleep 10
            echo -n 'zipfian theta='$contention' tput: ' >> contention_test_result.txt
            print | awk '{s += $3} END {print s/6}' | tee -a contention_test_result.txt
        done
    done
done
