#include <stdbool.h>             // for bool, false, true
#include <stdint.h>              // for int32_t, uint8_t, uint16_t, uint32_t
#include <stdlib.h>              // for size_t, strtol, NULL
#include <string.h>              // for strcpy
#include <sys/signal.h>          // for signal, SIGINT, SIGTERM
#include <sys/socket.h>          // for recv
#include <sys/types.h>           // for ssize_t
#include <unistd.h>              // for getpid, close

#include "connection.h"          // for connection_socket_to_listen, connect...
#include "control_utils.h"       // for die
#include "custom_logger.h"       // for custom_log_error
#include "format.h"              // for request
#include "format_server_node.h"  // for node_update_ret_payload, format_serv...
#include "io.h"                  // for io_write_all
#include "node_server.h"         // for node_log_info, node_server_handle_re...
#include "routing.h"             // for routing_table_new, routing_table_print
#include "serving.h"             // for serving_free, serving_init, serving_...
#include "settings.h"            // for NODES_ALIASES, NODE_COUNT, SERVER_PORT

static node_server_t server;
static struct node children[NODE_COUNT];

static volatile bool keeprunning = true;

static void int_handler(int32_t dummy);

static void term_handler(int32_t dummy);

static int32_t parse_args(char** args, size_t argc, uint16_t* port, const char** node_name);

static int32_t update_node_state(int32_t port, const char* node_name);

static int32_t handle_request(int32_t conn_fd, void* data);

int32_t main(int32_t argc, char** argv) {
	int32_t node_server_fd;
	struct serving_data serving;
	uint16_t port;
	const char* node_name;

	signal(SIGINT, int_handler);
	signal(SIGTERM, term_handler);

	if (parse_args(argv, (size_t) argc, &port, &node_name)) {
		die("Failed to parse args");
	}

	if (routing_table_new(&server.routing, server.label)) {
		die("Failed to init routing table");
	}
	routing_table_print(&server.routing, server.label);

	node_server_fd = connection_socket_to_listen(port);

	if (node_server_fd < 0) {
		die("Failed to start server on node %d", server.label);
	}

	if (update_node_state(port, node_name)) {
		die("Failed to init node");
	}

	serving_init(&serving, node_server_fd, handle_request);

	node_log_info("Node started on port %d", port);

	while (keeprunning) {
		serving_poll(&serving, children);
	}

	node_log_info("Shutting down node pid = %d", getpid());
	serving_free(&serving);

	return 0;
}

static void int_handler(int32_t dummy) {
	(void) dummy;
	keeprunning = false;
}

static void term_handler(int32_t dummy) {
	int_handler(dummy);
}

static int32_t parse_args(char** args, size_t argc, uint16_t* port, const char** node_name) {
	char* endptr;

	if (argc != 2) {
		return -1;
	}

	endptr = NULL;
	server.label = (int32_t) strtol(args[1], &endptr, 10);
	if (args[1]  == endptr) {
		return -1;
	}
	*port = node_port(server.label);
	*node_name = NODES_ALIASES[server.label];

	return 0;
}

static int32_t update_node_state(int32_t port, const char* node_name) {
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
	strcpy(payload.alias, node_name);

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
	char buf[256];
	int32_t res;

	received_bytes = recv(conn_fd, buf, sizeof(buf), 0);

	node_log_debug("Received %d bytes from client %d.", received_bytes, conn_fd, buf);

	res = 0;
	if (received_bytes > 0) {
		node_server_handle_request(&server, conn_fd, (uint8_t*) buf, received_bytes, data);
	} else {
		res = -1;
	}

	return res;
}
