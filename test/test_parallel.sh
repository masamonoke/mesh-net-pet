. ./common.sh --source-only

cd ..

# run server beforehand

test() {
	test_send 1 99 0
	test_send 50 39 0
	test_send 20 99 0
	test_send 56 78 0
	test_send 12 34 0
	test_send 98 0 0
	test_send 0 98 0
	test_send 45 23 0
	test_send 45 23 0
	test_send 20 25 0
	test_send 0 1 0
	test_send 0 99 0
	test_send 99 0 0
	test_send 34 56 0
	test_send 12 87 0
	test_send 45 23 0
	test_send 29 23 0
	test_send 1 67 0
	test_send 45 23 0
	test_send 55 89 0
	test_send 45 4 0
	test_send 10 2 0
	test_send 4 23 0
	test_send 4 3 0
	test_send 0 1 0
	test_send 56 23 0
	test_send 45 56 0
}

echo "Testing parallel clients"

reset_mesh

test & test & test

reset_mesh
