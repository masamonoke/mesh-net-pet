#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "control_utils.h"
#include "custom_logger.h"
#include "format_client_server.h"
#include "format_server_node.h"
#include "connection.h"
#include "serving.h"
#include "io.h"
#include "format.h"
#include "settings.h"

struct node {
	pid_t pid;
	int32_t write_fd;
	char* alias;
	int32_t label;
	int32_t port;
	bool initialized;
};

typedef struct server {
	struct serving_data serving;
	struct node children[NODE_COUNT];
} server_t;

static volatile bool keeprunning = true;

static void run_node(uint32_t node_label);

static server_t server_data;

static void int_handler(int32_t dummy);

static void term_handler(int32_t dummy);

static int32_t handle_request(int32_t conn_fd, void* data);

int32_t main(void) {
	size_t i;
	int32_t server_fd;

	signal(SIGINT, int_handler);
	signal(SIGTERM, term_handler);

	server_fd = connection_socket_to_listen(SERVER_PORT);

	if (server_fd < 0) {
		die("Failed to create server");
	}

	for (i = 0; i < NODE_COUNT; i++) {
		server_data.children[i].pid = fork();
		if (server_data.children[i].pid < 0) {
			custom_log_error("Failed to create child process");
			goto L_FREE;
		} else if (server_data.children[i].pid == 0) {
			run_node((uint32_t) i);
		} else {
			// parent
			server_data.children[i].initialized = false;
			server_data.children[i].write_fd = -1;
			server_data.children[i].port = -1;
			server_data.children[i].label = -1;
			server_data.children[i].alias = NULL;
		}
	}

	custom_log_info("Started server on port %d (process %d)", SERVER_PORT, getpid());

	serving_init(&server_data.serving, server_fd, handle_request);

	while (keeprunning) {
		serving_poll(&server_data.serving, server_data.children);
	}

L_FREE:

	serving_free(&server_data.serving);

	return 0;
}

static void run_node(uint32_t node_label) {
	char node_label_str[16];
	int32_t len;

	len = snprintf(node_label_str, sizeof(node_label_str), "%d", node_label);
	if (len < 0 || (size_t) len > sizeof(node_label_str)) {
		die("Failed to convert node_label to str");
	}

	execl("../node/mesh_node", "mesh_node", "-nodelabel", node_label_str, (char*) NULL);
}

static void int_handler(int32_t dummy) {
	(void) dummy;
	keeprunning = false;
}

static void term_handler(int32_t dummy) {
	size_t i;
	for (i = 0; i < NODE_COUNT; i++) {
		kill(server_data.children[i].pid, SIGINT);
	}
	int_handler(dummy);
}

static void update_child(void* payload, void* data);

static bool handle_client_request(int32_t client_fd, enum request_type_server_client* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data);

static bool handle_node_request(enum request_type_server_node* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data);

static int32_t handle_request(int32_t conn_fd, void* data) {
	ssize_t received_bytes;
	enum request_type_server_client client_server_cmd_type;
	enum request_type_server_node server_node_cmd_type;
	void* payload;
	char buf[256];

	received_bytes = recv(conn_fd, buf, sizeof(buf), 0);

	custom_log_debug("Received %d bytes from client %d", received_bytes, conn_fd);

	if (received_bytes > 0) {
		bool processed;

		processed = handle_client_request(conn_fd, &client_server_cmd_type, &payload, (uint8_t*) buf, (size_t) received_bytes, data) ||
			handle_node_request(&server_node_cmd_type, &payload, (uint8_t*) buf, (size_t) received_bytes, data);

		if (processed) {
			return 0;
		} else {
			custom_log_error("Failed to parse request");
			return -1;
		}
	}

	return -1;
}

static void ping(int32_t client_fd, void** payload);

static bool handle_client_request(int32_t client_fd, enum request_type_server_client* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data) {
	(void) data;
	format_server_client_parse_message(cmd_type, payload, buf, received_bytes);
	switch (*cmd_type) {
		case REQUEST_TYPE_SERVER_CLIENT_SEND_AS_NODE:
			not_implemented();
			break;
		case REQUEST_TYPE_SERVER_CLIENT_PING_NODE:
			ping(client_fd, payload);
			break;
		case REQUEST_TYPE_SERVER_CLIENT_UNDEFINED:
			return false;
			break;
	}

	return true;
}

static void ping(int32_t client_fd, void** payload) {
	size_t i;
	struct node_ping_ret_payload* p;
	bool found;
	struct timeval tv;

	found = false;
	p = (struct node_ping_ret_payload*) *payload;
	for (i = 0; i < NODE_COUNT; i++) {
		if (server_data.children[i].label == p->label) {
			uint8_t b[256];
			uint32_t buf_len;
			ssize_t received;

			format_server_node_create_message(REQUEST_TYPE_SERVER_NODE_PING, NULL, b, &buf_len);
			if (io_write_all(server_data.children[i].write_fd, (char*) b, buf_len)) {
				custom_log_error("Failed to send request to node");
			}
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			setsockopt(server_data.children[i].write_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

			received = recv(server_data.children[i].write_fd, b, sizeof(b), 0);

			tv.tv_sec = 0;
			tv.tv_usec = 0;
			setsockopt(server_data.children[i].write_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

			if (received > 0) {
				if (io_write_all(client_fd, (char*) b, (size_t) received)) {
					custom_log_error("Failed to send ping result to client");
				}
			} else {
				custom_log_error("Failed to get response from node %d", p->label);
			}

			found = true;
			break;
		}
	}

	if (!found) {
		enum request_result res;

		res = REQUEST_ERR;
		if (io_write_all(client_fd, (char*) &res, sizeof(res))) {
			custom_log_error("Failed to send ping result to client");
		}
	}
	free(p);
}

static bool handle_node_request(enum request_type_server_node* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data) {

	format_server_node_parse_message(cmd_type, payload, buf, received_bytes);
	switch (*cmd_type) {
		case REQUEST_TYPE_SERVER_NODE_UPDATE:
			update_child(*payload, data);
			break;
		case REQUEST_TYPE_SERVER_NODE_PING:
			not_implemented();
			break;
		case REQUEST_TYPE_SERVER_NODE_UNDEFINED:
			return false;
			break;
	}

	return true;
}

static void update_child(void* payload, void* data) { // NOLINT
	struct node_update_ret_payload* ret;
	size_t i;
	struct node* children;

	children = (struct node*) data;

	ret = (struct node_update_ret_payload*) payload;

	for (i = 0; i < NODE_COUNT; i++) {
		if (children[i].pid == ret->pid) {
			children[i].port = ret->port;
			children[i].label = ret->label;
			children[i].alias = ret->alias;
			children[i].initialized = true;

			children[i].write_fd = connection_socket_to_send((uint16_t) children[i].port);
			if (children[i].write_fd < 0) {
				custom_log_error("Failed to establish connection with node port=%d, alias=%s", children[i].port, children[i].alias);
			} else {
				custom_log_info("Established connection with node: label=%d, alias=%s", children[i].label, children[i].alias);
			}
			break;
		}
	}
	free(payload);
}
