#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "custom_logger.h"
#include "control_utils.h"
#include "connection.h"
#include "format_server_node.h"
#include "io.h"
#include "serving.h"
#include "format.h"
#include "settings.h"
#include "routing.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#define node_log_info(format, ...) custom_log_info("Node %s l=%d, p=%d: " format, node_name, label, port, ##__VA_ARGS__)
#define node_log_error(format, ...) custom_log_error("Node %s l=%d, p=%d: " format, node_name, label, port, ##__VA_ARGS__)
#define node_log_warn(format, ...) custom_log_warn("Node %s l=%d, p=%d: " format, node_name, label, port, ##__VA_ARGS__)
#define node_log_debug(format, ...) custom_log_debug("Node %s l=%d, p=%d: " format, node_name, label, port, ##__VA_ARGS__)

#pragma clang diagnostic pop

#else

#define node_log_info(format, ...) custom_log_info("Node %s: " format, node_name, ##__VA_ARGS__)
#define node_log_error(format, ...) custom_log_error("Node %s: " format, node_name, ##__VA_ARGS__)
#define node_log_warn(format, ...) custom_log_warn("Node %s: " format, node_name, ##__VA_ARGS__)
#define node_log_debug(format, ...) custom_log_debug("Node %s: " format, node_name, ##__VA_ARGS__)

#endif

typedef struct server {
	struct serving_data serving;
	struct routing_table_t* table;
} server_t;

static const char* node_name;
static int32_t label;
static uint16_t port;
static routing_table_t routing;

static volatile bool keeprunning = true;

static void int_handler(int32_t dummy);

static void term_handler(int32_t dummy);

static int32_t parse_args(char** args, size_t argc);

static void update_node_state(void);

static int32_t handle_request(int32_t conn_fd, void* data);

int32_t main(int32_t argc, char** argv) {
	int32_t server_fd;
	server_t server_data;

	signal(SIGINT, int_handler);
	signal(SIGTERM, term_handler);

	if (parse_args(argv, (size_t) argc)) {
		die("Failed to parse args");
	}

	routing_table_new(&routing, label);
	routing_table_print(&routing, label);

	server_fd = connection_socket_to_listen(port);

	update_node_state();

	serving_init(&server_data.serving, server_fd, handle_request);

	node_log_info("Node started on port %d", port);

	while (keeprunning) {
		serving_poll(&server_data.serving, NULL);
	}

	node_log_info("Shutting down node pid = %d", getpid());
	serving_free(&server_data.serving);

	return 0;
}

static void int_handler(int32_t dummy) {
	(void) dummy;
	keeprunning = false;
}

static void term_handler(int32_t dummy) {
	int_handler(dummy);
}

static int32_t parse_args(char** args, size_t argc) {
	char* endptr;

	if (argc != 2) {
		return -1;
	}

	endptr = NULL;
	label = (int32_t) strtol(args[1], &endptr, 10);
	if (args[1]  == endptr) {
		return -1;
	}
	port = node_port(label);
	node_name = NODES_ALIASES[label];

	return 0;
}

static void update_node_state(void) {
	uint8_t buf[256];
	uint32_t buf_len;
	int32_t server_fd;

	server_fd = connection_socket_to_send(SERVER_PORT);

	if (server_fd < 0) {
		die("Failed to get socket");
	}

	struct node_update_ret_payload payload = {
		.label = label,
		.pid = getpid(),
		.port = port,
	};
	strcpy(payload.alias, node_name);
	format_server_node_create_message(REQUEST_TYPE_SERVER_NODE_UPDATE, &payload, buf, &buf_len);
	if (io_write_all(server_fd, (const char*) buf, buf_len)) {
		node_log_error("Failed to send node init data");
	}

	close(server_fd);
}

static bool handle_server_request(int32_t conn_fd, enum request_type_server_node* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data);

static int32_t handle_request(int32_t conn_fd, void* data) {
	(void) data;
	ssize_t received_bytes;
	char buf[256];
	enum request_type_server_node server_node_cmd_type;
	void* payload;


	received_bytes = recv(conn_fd, buf, sizeof(buf), 0);

	node_log_debug("Received %d bytes from client %d.", received_bytes, conn_fd, buf);

	if (received_bytes > 0) {
		handle_server_request(conn_fd, &server_node_cmd_type, &payload, (uint8_t*) buf, (size_t) received_bytes, data);
	} else {
		return -1;
	}

	return 0;
}

static bool handle_server_request(int32_t conn_fd, enum request_type_server_node* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data) {
	(void) data;
	format_server_node_parse_message(cmd_type, payload, buf, received_bytes);
	switch (*cmd_type) {
		case REQUEST_TYPE_SERVER_NODE_PING:
			{
				enum request_result res;

				res = REQUEST_OK;
				if (io_write_all(conn_fd, (char*) &res, sizeof(res))) {
					node_log_error("Failed to response to ping");
				}
			}
			break;
		case REQUEST_TYPE_SERVER_NODE_UNDEFINED:
			custom_log_error("Undefined server-node request type");
			return false;
		default:
			return false;
	}

	return true;
}
