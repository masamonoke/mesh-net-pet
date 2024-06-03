cd ..

timeout() {
	local cmd_pid sleep_pid retval
	(shift; "$@") &   # shift out sleep value and run rest as command in background job
	cmd_pid=$!
	(sleep "$1"; kill "$cmd_pid" 2>/dev/null) &
	sleep_pid=$!
	wait "$cmd_pid"
	retval=$?
	kill "$sleep_pid" 2>/dev/null
	return "$retval"
}

function benchmark() {
	sum=0
	while true; do
		addr_s=$((0 + $RANDOM % 99))
		addr_r=$((0 + $RANDOM % 99))
		make client TARGET_ARGS="send -s $addr_s -r $addr_r -a 'Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Aenean commodo ligula eget dolor. Aenean massa. Cum sociis natoque penatibus et magnis dis' -as 2 -ar 3" > /dev/null 2>&1
		sum=$((sum + 1))
		echo $sum 
	done
}

echo "Throughput benchmark"

echo "Requests per 10 seconds in reset network"

make client TARGET_ARGS="reset" > /dev/null 2>&1

timeout 10 benchmark

make client TARGET_ARGS="reset" > /dev/null 2>&1
