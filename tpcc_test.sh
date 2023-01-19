print() {
for line in {28..81};
do 
    arg=$line"p"
    sed -n $arg ./output.txt
done
}

for contention in 1 3 5 7 10 15 20;
# for contention in 0 0.99 ;
do
    for i in {1};
    do
        bin/deployment/cluster -c deploy-run.conf -p src/deployment/portfile -d bin/deployment/db t $contention 0 | tee output.txt &
        sleep 120
        pkill cluster
        sleep 10
        echo -n 'remote param='$contention' tput: ' >> contention_test_result.txt
        print | awk '{s += $3} END {print s/6}' | tee -a contention_test_result.txt
    done
done

