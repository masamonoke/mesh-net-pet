#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection.h"
#include "control_utils.h"
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

__attribute__((warn_unused_result))
static bool parse_args(char** args, size_t argc, uint16_t* port);

__attribute__((warn_unused_result))
static bool update_node_state(uint16_t port);

__attribute__((warn_unused_result))
static bool handle_request(int32_t conn_fd, void* data);

int32_t main(int32_t argc, char** argv) {
	int32_t node_server_fd;
	struct serving_data serving;
	uint16_t port;

	signal(SIGINT, int_handler);
	signal(SIGTERM, term_handler);

	if (!parse_args(argv, (size_t) argc, &port)) {
		die("Failed to parse args");
	}

	routing_table_fill_default(&server.routing);
	node_app_fill_default(server.apps, server.addr);
	node_essentials_fill_neighbors_port(server.addr);

	node_server_fd = connection_socket_to_listen(port);

	if (node_server_fd < 0) {
		die("Failed to start server on node %d", server.addr);
	}

	if (!update_node_state(port)) {
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

static bool parse_args(char** args, size_t argc, uint16_t* port) {
	char* endptr;

	if (argc != 2) {
		return false;
	}

	endptr = NULL;
	server.addr = (uint8_t) strtol(args[1], &endptr, 10);
	if (args[1]  == endptr) {
		return false;
	}
	*port = node_port(server.addr);

	return true;
}

static bool update_node_state(uint16_t port) {
	uint8_t buf[UPDATE_LEN + MSG_LEN];
	msg_len_type buf_len;
	int32_t server_fd;
	bool status;

	status = true;
	server_fd = connection_socket_to_send(SERVER_PORT);

	if (server_fd < 0) {
		die("Failed to get socket");
	}

	node_update_t payload = {
		.addr = server.addr,
		.pid = getpid(),
		.port = port,
	};

	format_server_node_create_message(REQUEST_UPDATE, &payload, buf, &buf_len);

	if (!io_write_all(server_fd, buf, buf_len)) {
		node_log_error("Failed to send node init data");
		status = false;
	}

	close(server_fd);

	return status;
}

static bool handle_request(int32_t conn_fd, void* data) {
	int16_t received_bytes;
	uint8_t buf[MAX_MSG_LEN];
	uint8_t msg_len;

	if (!io_read_all(conn_fd, &msg_len, sizeof(msg_len), &received_bytes)) {
		node_log_error("Failed to read message length");
	}

	if (received_bytes > 0) {
		if (!io_read_all(conn_fd, buf, msg_len - sizeof(msg_len), &received_bytes)) {
			node_log_error("Failed to read message");
			return false;
		}
		if (!format_is_message_correct((size_t) received_bytes, msg_len - sizeof(msg_len))) {
			node_log_error("Incorrect message");
			return false;
		}
	} else {
		return false;
	}

	if (received_bytes > 0) {
		node_server_handle_request(&server, conn_fd, (uint8_t*) buf, received_bytes, data);
	} else {
		node_log_error("Failed to read %d bytes", msg_len);
		return false;
	}

	return true;
}
