#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "connection.h"
#include "control_utils.h"
#include "custom_logger.h"
#include "format.h"
#include "format_server_node.h"
#include "io.h"
#include "node_server.h"
#include "routing.h"
#include "serving.h"
#include "settings.h"
#include "node_essentials.h"
#include "node_app.h"

static node_server_t server;
static struct node children[NODE_COUNT];

static volatile bool keeprunning = true;

static void int_handler(int32_t dummy);

static void term_handler(int32_t dummy);

static int32_t parse_args(char** args, size_t argc, uint16_t* port);

static int32_t update_node_state(int32_t port);

static int32_t handle_request(int32_t conn_fd, void* data);

int32_t main(int32_t argc, char** argv) {
	int32_t node_server_fd;
	struct serving_data serving;
	uint16_t port;

	signal(SIGINT, int_handler);
	signal(SIGTERM, term_handler);

	if (parse_args(argv, (size_t) argc, &port)) {
		die("Failed to parse args");
	}

	if (routing_table_fill_default(&server.routing)) {
		die("Failed to init routing table");
	}

	node_app_fill_default(server.apps, server.label);

	node_server_fd = connection_socket_to_listen(port);

	if (node_server_fd < 0) {
		die("Failed to start server on node %d", server.label);
	}

	if (update_node_state(port)) {
		die("Failed to init node");
	}

	serving_init(&serving, node_server_fd, handle_request);

	while (keeprunning) {
		serving_poll(&serving, children);
	}

	serving_free(&serving);
	close(node_server_fd);
	node_essentials_reset_connections();

	node_log_debug("Killed node process %d", getpid());

	return 0;
}

static void int_handler(int32_t dummy) {
	(void) dummy;
	keeprunning = false;
}

static void term_handler(int32_t dummy) {
	int_handler(dummy);
}

static int32_t parse_args(char** args, size_t argc, uint16_t* port) {
	char* endptr;

	if (argc != 2) {
		return -1;
	}

	endptr = NULL;
	server.label = (int8_t) strtol(args[1], &endptr, 10);
	if (args[1]  == endptr) {
		return -1;
	}
	*port = node_port(server.label);

	return 0;
}

static int32_t update_node_state(int32_t port) {
	uint8_t buf[256];
	uint32_t buf_len;
	int32_t server_fd;
	int32_t status;

	status = 0;
	server_fd = connection_socket_to_send(SERVER_PORT);

	if (server_fd < 0) {
		die("Failed to get socket");
	}

	struct node_update_ret_payload payload = {
		.label = server.label,
		.pid = getpid(),
		.port = port,
	};

	if (format_server_node_create_message(REQUEST_UPDATE, &payload, buf, &buf_len)) {
		custom_log_error("Failed to create message");
		status = -1;
	}

	if (io_write_all(server_fd, (const char*) buf, buf_len)) {
		node_log_error("Failed to send node init data");
		status = -1;
	}

	close(server_fd);

	return status;
}

static int32_t handle_request(int32_t conn_fd, void* data) {
	ssize_t received_bytes;
	uint8_t buf[256];
	int32_t res;
	uint32_t msg_len;


	if (io_read_all(conn_fd, (char*) &msg_len, sizeof(msg_len), (size_t*) &received_bytes)) {
		node_log_error("Failed to read message length");
	}

	if (received_bytes > 0) {
		if (io_read_all(conn_fd, (char*) buf, msg_len - sizeof(uint32_t), (size_t*) &received_bytes)) {
			node_log_error("Failed to read message");
		}
		if (!format_is_message_correct((size_t) received_bytes, msg_len - sizeof(msg_len))) {
			node_log_error("Incorrect message");
		}
	} else {
		return -1;
	}

	res = 0;
	if (received_bytes > 0) {
		node_server_handle_request(&server, conn_fd, (uint8_t*) buf, received_bytes, data);
	} else {
		node_log_error("Failed to read %d bytes", msg_len);
		res = -1;
	}

	return res;
}
