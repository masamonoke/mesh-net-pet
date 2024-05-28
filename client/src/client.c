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

	status = REQUEST_UNKNOWN;
	if (format_server_client_create_message(req, payload, buf, &buf_len)) {
		custom_log_error("Failed to create format message");
		goto L_FREE;
	}

	if (io_write_all(server_fd, (const char*) buf, buf_len)) {
		custom_log_error("Failed to send client command");
	}

	tv.tv_sec = 1;
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
	free(payload);
	close(server_fd);

	return (int32_t) status;
}

static int32_t create_label_payload(const char* arg, void** payload) {
	char* endptr;
	int8_t label;

	endptr = NULL;
	label = (int8_t) strtol(arg, &endptr, 10);
	if (arg == endptr) {
		return -1;
	}

	*payload = malloc(sizeof(struct node_label_payload));
	((struct node_label_payload*) *payload)->label = label;

	return 0;
}

static int32_t parse_args(int32_t argc, char** argv, enum request* cmd, void** payload) {
	int32_t i;

	if (argc > 2) {
		for (i = 0; i < argc; i++) {
			if (0 == strcmp(argv[i], "send") && argc >= 6) {
				int8_t label_to;
				int8_t label_from;
				char* endptr;


				*cmd = REQUEST_SEND;
				label_to = -1;
				label_from = -1;
				for (i = 0; i < argc; i++) {
					if (0 == strcmp(argv[i], "-s") || 0 == strcmp(argv[i], "--sender")) {
						endptr = NULL;
						label_from = (int8_t) strtol(argv[i + 1], &endptr, 10);
						if (argv[i + 1] == endptr) {
							return -1;
						}
					}
					if (0 == strcmp(argv[i], "-r") || 0 == strcmp(argv[i], "--receiver")) {
						endptr = NULL;
						label_to = (int8_t) strtol(argv[i + 1], &endptr, 10);
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
				*cmd = REQUEST_PING;
				return create_label_payload(argv[i + 1], payload);
			} else if (0 == strcmp(argv[i], "kill")) {
				*cmd = REQUEST_KILL_NODE;
				return create_label_payload(argv[i + 1], payload);
			} else if (0 == strcmp(argv[i], "revive")) {
				*cmd = REQUEST_REVIVE_NODE;
				return create_label_payload(argv[i + 1], payload);
			}
		}
	} else if (argc == 2) {
		if (0 == strcmp(argv[1], "reset")) {
			*cmd = REQUEST_RESET;
		}

		return 0;
	}

	return -1;
}
