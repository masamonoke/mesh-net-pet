#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection.h"
#include "control_utils.h"
#include "custom_logger.h"
#include "request.h"
#include "serving.h"
#include "settings.h"

static volatile bool keeprunning = true;

static void run_node(uint32_t node_label);

static server_t server_data;

static void int_handler(int32_t dummy);

static void term_handler(int32_t dummy);

static int32_t handle_request(int32_t conn_fd, void* data);

int32_t main(void) {
	size_t i;
	int32_t server_fd;
	struct serving_data serving;

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
		} else if (server_data.children[i].pid == 0) {
			run_node((uint32_t) i);
		} else {
			// parent
			server_data.children[i].initialized = false;
			server_data.children[i].write_fd = -1;
			server_data.children[i].port = -1;
			server_data.children[i].label = -1;
		}
	}
	server_data.client_fd = -1;

	custom_log_info("Started server on port %d (process %d)", SERVER_PORT, getpid());

	serving_init(&serving, server_fd, handle_request);

	while (keeprunning) {
		serving_poll(&serving, server_data.children);
	}

	serving_free(&serving);

	return 0;
}

static void run_node(uint32_t node_label) {
	char node_label_str[16];
	int32_t len;

	len = snprintf(node_label_str, sizeof(node_label_str), "%d", node_label);
	if (len < 0 || (size_t) len > sizeof(node_label_str)) {
		die("Failed to convert node_label to str");
	}

	execl("../node/mesh_node", "mesh_node", node_label_str, (char*) NULL);
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

static int32_t handle_request(int32_t conn_fd, void* data) {
	ssize_t received_bytes;
	char buf[256];

	received_bytes = recv(conn_fd, buf, sizeof(buf), 0);

	custom_log_debug("Received %d bytes from client %d", received_bytes, conn_fd);

	if (received_bytes > 0) {
		bool processed;

		processed = request_handle(&server_data, (uint8_t*) buf, received_bytes, conn_fd, data);

		if (processed) {
			return 0;
		} else {
			custom_log_error("Failed to parse request");
			return -1;
		}
	}

	return -1;
}
