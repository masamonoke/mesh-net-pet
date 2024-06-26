#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "connection.h"
#include "control_utils.h"
#include "custom_logger.h"
#include "server_listener.h"
#include "serving.h"
#include "settings.h"
#include "io.h"
#include "format.h"
#include "server_essentials.h"

static volatile bool keeprunning = true;

static server_t server_data;

static void int_handler(int32_t dummy);

static void term_handler(int32_t dummy);

static bool handle_request(int32_t conn_fd, void* data);

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

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		server_data.children[i].pid = fork();
		if (server_data.children[i].pid < 0) {
			custom_log_error("Failed to create child process");
		} else if (server_data.children[i].pid == 0) {
			run_node((uint8_t) i);
			perror("execl()");
			exit(EXIT_FAILURE);
		} else {
			// parent
			server_data.children[i].write_fd = -1;
			server_data.children[i].port = UINT16_MAX;
			server_data.children[i].addr = UINT8_MAX;
		}
	}
	server_data.client_fd = -1;

	custom_log_debug("Started server on port %d (process %d)", SERVER_PORT, getpid());
	custom_log_info("-------------------------------New session-------------------------------");

	serving_init(&serving, server_fd, handle_request);
	server_listener_init();

	while (keeprunning) {
		serving_poll(&serving, server_data.children);
	}

	serving_free(&serving);

	return 0;
}

static void int_handler(int32_t dummy) {
	(void) dummy;
	keeprunning = false;
}

static void term_handler(int32_t dummy) {
	size_t i;
	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (server_data.children[i].pid > 0) {
			kill(server_data.children[i].pid, SIGINT);
		}
	}
	int_handler(dummy);
}

static bool handle_request(int32_t conn_fd, void* data) {
	int16_t received_bytes;
	uint8_t buf[MAX_MSG_LEN];
	uint8_t msg_len;

	msg_len = 0;

	if (!io_read_all(conn_fd, &msg_len, sizeof(msg_len), &received_bytes)) {
		custom_log_error("Failed to read message length");
		return false;
	}

	if (received_bytes > 0) {
		if (!io_read_all(conn_fd, buf, msg_len - sizeof(msg_len), &received_bytes)) {
			custom_log_error("Failed to read message");
			return false;
		}

		if (!format_is_message_correct((size_t) received_bytes, msg_len - sizeof(msg_len))) {
			custom_log_error("Incorrect message format");
			return false;
		}
	} else {
		return false;
	}

	if (received_bytes > 0) {
		bool processed;

		processed = server_listener_handle(&server_data, (uint8_t*) buf, conn_fd, data);

		if (processed) {
			return true;
		} else {
			custom_log_error("Failed to parse request");
			return false;
		}
	} else {
		custom_log_error("Failed to receive message: declared length %d", msg_len);
	}

	return false;
}
