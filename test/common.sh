reset_mesh() {
	make client TARGET_ARGS="reset" > /dev/null 2>&1
}

kill_node() {
	make client TARGET_ARGS="kill $1" > /dev/null 2>&1
}

test_send() {
	make client TARGET_ARGS="send -s $1 -r $2" > /dev/null 2>&1
	if [ $? != $3 ]; then
		echo "Failed: send from $1 to $2 but returned $?"
	else
		echo "Passed: send from $1 to $2 returned $3"
	fi
}
