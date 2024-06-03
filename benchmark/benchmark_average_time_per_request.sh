cd ..

count_time() {
	START=$(($(gdate +%s%N) / 1000000))
	make client TARGET_ARGS="send -s $1 -r $2 -a 'Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis' -as 2 -ar 3" > /dev/null 2>&1
	END=$(($(gdate +%s%N) / 1000000))
	DIFF=$(echo "$END - $START" | bc)
	echo $DIFF
}

benchmark() {
	sum=0
	n=70
	for i in $(seq 1 $n);
	do
		addr_s=$((0 + $RANDOM % 99))
		addr_r=$((0 + $RANDOM % 99))
		t=$(count_time $addr_r $addr_s)
		sum=$((sum + t))
	done

	echo Average in miliseconds: $((sum / n))
}

echo "Average time per request benchmark"

make client TARGET_ARGS="reset" > /dev/null 2>&1

echo "Benchmarking sending in reset network"

benchmark

echo "Benchmarking sending in path found network"

benchmark

make client TARGET_ARGS="reset" > /dev/null 2>&1
