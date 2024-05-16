#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "custom_logger.h"
#include "control_utils.h"
#include "connection.h"
#include "format_client_server.h"
#include "io.h"
#include "format.h"
#include "settings.h"

static int32_t parse_args(int32_t argc, char** argv, enum request_type_server_client* cmd, void** payload);

int32_t main(int32_t argc, char** argv) {
	int32_t server_fd;
	enum request_type_server_client req;
	uint8_t buf[256];
	uint32_t buf_len;
	void* payload;
	ssize_t received_bytes;
	enum request_result status;
	struct timeval tv;

	if (parse_args(argc, argv, &req, &payload)) {
		custom_log_error("Failed to parse client args");
		return 1;
	}

	server_fd = connection_socket_to_send(SERVER_PORT);

	if (server_fd < 0) {
		die("Failed to get socket");
	}

	format_server_client_create_message(req, payload, buf, &buf_len);
	if (io_write_all(server_fd, (const char*) buf, buf_len)) {
		custom_log_error("Failed to send client command");
	}

	tv.tv_sec = 2;
	tv.tv_usec = 0;
	setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	status = REQUEST_UNKNOWN;

	received_bytes = recv(server_fd, buf, sizeof(buf), 0);
	if (received_bytes > 0) {
		memcpy(&status, buf, sizeof(status));
	}

	format_sprint_result(status, (char*) buf, sizeof(buf));
	custom_log_info("Ping result %s", buf);

	close(server_fd);
	free(payload);

	return 0;
}

static int32_t parse_args(int32_t argc, char** argv, enum request_type_server_client* cmd, void** payload) {
	int32_t i;

	if (argc > 2) {
		for (i = 0; i < argc; i++) {
			if (0 == strcmp(argv[i], "send")) {
				*cmd = REQUEST_TYPE_SERVER_CLIENT_SEND_AS_NODE;
				return 0;
			}
			if (0 == strcmp(argv[i], "ping")) {
				char* endptr;
				int32_t label;

				endptr = NULL;
				label = (int32_t) strtol(argv[i + 1], &endptr, 10);
				if (argv[i + 1] == endptr) {
					return -1;
				}

				*cmd = REQUEST_TYPE_SERVER_CLIENT_PING_NODE;
				*payload = malloc(sizeof(struct node_ping_ret_payload));
				((struct node_ping_ret_payload*) *payload)->label = label;

				return 0;
			}
		}
	}

	return -1;
}
