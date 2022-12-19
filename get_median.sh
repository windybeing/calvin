for i in {0..14};
do
    for ((line=$i*5+1; line<$i*5+6; line+=1));
    do 
        arg=$line"p"
        sed -n $arg ./contention_test_result.txt | awk {'print $4'} >> tmp.txt
    done
    sort -n tmp.txt | sed -n 3p 
    :>tmp.txt
done
