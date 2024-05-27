echo "Testing partially broken network along side diagonal"

matrix_size=$((10))

label() {
	echo  $(($1 * $((matrix_size)) + $2))
}

kill_node() {
	make client TARGET_ARGS="kill $1" > /dev/null 2>&1
}

setup() {
	for i in $(seq 1 10);
	do
		for j in $(seq 1 10)
		do
			if [ $(((i) + (j))) = $((matrix_size - 1)) ]; then
				l=$(label $i $j)
				kill_node $l
			fi
		done
	done

	sleep 5
}

. ./common.sh --source-only

cd ..

setup
# TODO: can be broken pipe (141) after killing

send 0 99 0
send 99 0 0
send 0 45 2
send 1 99 0
send 1 100 2
send 50 39 0
send 20 99 0
send 56 78 0
send 12 34 0
send 98 0 0
send 0 98 0
send 20 25 0
send 0 1 0
send 0 99 0
send 99 0 0
send 34 56 0
send 12 87 0
send -1 23 2

teardown
