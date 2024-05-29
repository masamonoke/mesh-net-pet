echo "Testing healthy network"

. ./common.sh --source-only

cd ..

# run server beforehand

reset_mesh

test_send 1 99 0
test_send 1 100 2
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
test_send -1 23 2

reset_mesh
