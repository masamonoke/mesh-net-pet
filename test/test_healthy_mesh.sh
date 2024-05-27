echo "Testing healthy network"

. ./common.sh --source-only

cd ..

# run server beforehand

send 1 99 0
send 1 100 2
send 50 39 0
send 20 99 0
send 56 78 0
send 12 34 0
send 98 0 0
send 0 98 0
send 45 23 0
send 45 23 0
send 20 25 0
send 0 1 0
send 0 99 0
send 99 0 0
send 34 56 0
send 12 87 0
send -1 23 2

teardown
