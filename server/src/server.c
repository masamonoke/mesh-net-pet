#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "control_utils.h"
#include "custom_logger.h"
#include "format.h"
#include "connection.h"
#include "serving.h"

// TODO: read nodes count, aliases, server port from config or env

#ifndef SERVER_PORT
#define SERVER_PORT 999
#endif

#ifndef NODE_COUNT
#define NODE_COUNT 4
#endif

// fields alias and is_alive needed for debug only
struct node {
	pid_t pid;
	int32_t fd;
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

static bool is_parent(const struct node* children, size_t len);

static void int_handler(int32_t dummy);

static void server_data_init(server_t* server_data);

static void server_data_free(server_t* server_data);

static int32_t handle_request(int32_t conn_fd, void* children);

int32_t main(void) {
	size_t i;
	server_t server_data;

	signal(SIGINT, int_handler);

	if (server_data.serving.server_fd < 0) {
		die("Failed to get socket");
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
			server_data.children[i].fd = -1;
			server_data.children[i].port = -1;
			server_data.children[i].label = -1;
		}
	}

	if (is_parent(server_data.children, NODE_COUNT)) {
		custom_log_info("Started server on port %d (process %d)", SERVER_PORT, getpid());
	}

	server_data_init(&server_data);

	while (keeprunning) {
		// TODO: make non block on accept
		serving_poll(&server_data.serving, server_data.children);
	}

L_FREE:

	server_data_free(&server_data);

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

static bool is_parent(const struct node* children, size_t len) {
	bool is_server;
	size_t i;

	is_server = true;
	for (i = 0; i < len; i++) {
		if (children[i].pid == 0) {
			is_server = false;
			break;
		}
	}

	return is_server;
}

static void int_handler(int32_t dummy) {
	(void) dummy;
	keeprunning = false;
}

static void update_child(void* payload, struct node* children);

static int32_t handle_request(int32_t conn_fd, void* data) {
	int32_t sender_fd;
	ssize_t received_bytes;
	enum request_type cmd_type;
	void* payload;
	char buf[256];
	struct node* children;

	children = (struct node*) data;
	received_bytes = recv(conn_fd, buf, sizeof(buf), 0);

	sender_fd = conn_fd;

	custom_log_debug("Received %d bytes from client %d", received_bytes, sender_fd);

	if (received_bytes > 0) {
		format_parse_message(&cmd_type, &payload, (uint8_t*) buf, (size_t) received_bytes);
		switch (cmd_type) {
			case REQUEST_TYPE_NODE_UPDATE:
				update_child(payload, children);
				break;
			case REQUEST_TYPE_SEND_AS_NODE:
			case REQUEST_TYPE_PING_NODE:
				not_implemented();
				break;
			case REQUEST_TYPE_UNDEFINED:
				custom_log_error("Failed to parse request");
				break;
		}
	} else {
		return -1;
	}

	return 0;
}

static void update_child(void* payload, struct node* children) {
	struct node_upate_ret_payload* ret;
	size_t i;

	ret = (struct node_upate_ret_payload*) payload;
	custom_log_debug("Got node update");
	for (i = 0; i < NODE_COUNT; i++) {
		if (children[i].pid == ret->pid) {
			children[i].port = ret->port;
			children[i].label = ret->label;
			children[i].alias = ret->alias;
			children[i].initialized = true;
			custom_log_debug("Updated child %d data: label = %d, port = %d, alias = %s",
				children[i].pid, children[i].label, children[i].port, children[i].alias);
			// TODO: open connection with child
		}
	}
	free(payload);
}

static void server_data_init(server_t* server_data) {
	server_data->serving.server_fd = connection_socket_to_listen(SERVER_PORT);
	server_data->serving.handle_request = handle_request;

	server_data->serving.pfd_count = 0;
	server_data->serving.pfd_capacity = 5;
	server_data->serving.pfds = malloc(sizeof(struct pollfd) * server_data->serving.pfd_capacity);

	server_data->serving.pfds[0].fd = server_data->serving.server_fd;
    server_data->serving.pfds[0].events = POLLIN;
	server_data->serving.pfd_count++;
}

static void server_data_free(server_t* server_data) {
	size_t i;

	for (i = 0; i < server_data->serving.pfd_count; i++) {
		close(server_data->serving.pfds[i].fd);
	}
	free(server_data->serving.pfds);
}
