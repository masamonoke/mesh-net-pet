echo "Testing partially broken network along side diagonal"

matrix_size=$((10))

label() {
	echo  $(($1 * $((matrix_size)) + $2))
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

reset_mesh

setup
# TODO: can be broken pipe (141) after killing

test_send 0 99 0
test_send 99 0 0
test_send 0 45 2
test_send 1 99 0
test_send 1 100 2
test_send 50 39 0
test_send 20 99 0
test_send 56 78 0
test_send 12 34 0
test_send 98 0 0
test_send 0 98 0
test_send 20 25 0
test_send 0 1 0
test_send 0 99 0
test_send 99 0 0
test_send 34 56 0
test_send 12 87 0
test_send -1 23 2

reset_mesh
