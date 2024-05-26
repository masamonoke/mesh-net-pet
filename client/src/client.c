#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection.h"
#include "control_utils.h"
#include "custom_logger.h"
#include "format.h"
#include "format_client_server.h"
#include "io.h"
#include "settings.h"

static int32_t parse_args(int32_t argc, char** argv, enum request* cmd, void** payload);

int32_t main(int32_t argc, char** argv) {
	int32_t server_fd;
	enum request req;
	uint8_t buf[256];
	uint32_t buf_len;
	void* payload;
	ssize_t received_bytes;
	enum request_result status;
	struct timeval tv;

	payload = NULL;
	if (parse_args(argc, argv, &req, &payload)) {
		custom_log_error("Failed to parse client args");
		return 1;
	}

	server_fd = connection_socket_to_send(SERVER_PORT);

	if (server_fd < 0) {
		die("Failed to get socket");
	}

	if (format_server_client_create_message(req, payload, buf, &buf_len)) {
		custom_log_error("Failed to create format message");
		goto L_FREE;
	}

	if (io_write_all(server_fd, (const char*) buf, buf_len)) {
		custom_log_error("Failed to send client command");
	}

	tv.tv_sec = 10;
	tv.tv_usec = 0;
	setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	status = REQUEST_UNKNOWN;

	received_bytes = recv(server_fd, buf, sizeof(buf), 0);
	if (received_bytes > 0) {
		memcpy(&status, buf, sizeof(status));
	}

	format_sprint_result(status, (char*) buf, sizeof(buf));
	custom_log_info("Request result %s", buf);

L_FREE:
	close(server_fd);
	free(payload);

	return 0;
}

static int32_t parse_args(int32_t argc, char** argv, enum request* cmd, void** payload) {
	int32_t i;

	if (argc > 2) {
		for (i = 0; i < argc; i++) {
			if (0 == strcmp(argv[i], "send") && argc >= 6) {
				int32_t label_to;
				int32_t label_from;
				char* endptr;


				*cmd = REQUEST_SEND;
				label_to = -1;
				label_from = -1;
				for (i = 0; i < argc; i++) {
					if (0 == strcmp(argv[i], "-s") || 0 == strcmp(argv[i], "--sender")) {
						endptr = NULL;
						label_from = (int32_t) strtol(argv[i + 1], &endptr, 10);
						if (argv[i + 1] == endptr) {
							return -1;
						}
					}
					if (0 == strcmp(argv[i], "-r") || 0 == strcmp(argv[i], "--receiver")) {
						endptr = NULL;
						label_to = (int32_t) strtol(argv[i + 1], &endptr, 10);
						if (argv[i + 1] == endptr) {
							return -1;
						}
					}
				}

				if (label_to < 0 || label_from < 0) {
					custom_log_error("Failed to parse send command");
					return -1;
				}

				*payload = malloc(sizeof(struct send_to_node_ret_payload));
				((struct send_to_node_ret_payload*) *payload)->label_from = label_from;
				((struct send_to_node_ret_payload*) *payload)->label_to = label_to;

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

				*cmd = REQUEST_PING;
				*payload = malloc(sizeof(struct node_ping_ret_payload));
				((struct node_ping_ret_payload*) *payload)->label = label;

				return 0;
			}
		}
	}

	return -1;
}
