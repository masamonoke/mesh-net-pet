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

__attribute__((warn_unused_result))
static bool parse_args(int32_t argc, char** argv, enum request* cmd, void** payload);

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
	if (!parse_args(argc, argv, &req, &payload)) {
		custom_log_error("Failed to parse client args");
		return 1;
	}

	server_fd = connection_socket_to_send(SERVER_PORT);

	if (server_fd < 0) {
		die("Failed to get socket");
	}

	status = REQUEST_UNKNOWN;
	format_server_client_create_message(req, payload, buf, &buf_len);

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

	free(payload);
	close(server_fd);

	return (int32_t) status;
}

__attribute__((warn_unused_result))
static bool create_addr_payload(const char* arg, void** payload) {
	char* endptr;
	uint8_t addr;

	endptr = NULL;
	addr = (uint8_t) strtol(arg, &endptr, 10);
	if (arg == endptr) {
		return false;
	}

	*payload = malloc(sizeof(uint8_t));
	*((uint8_t*) *payload) = addr;

	return true;
}

static bool parse_args(int32_t argc, char** argv, enum request* cmd, void** payload) {
	int32_t i;

	if (argc > 2) {
		for (i = 0; i < argc; i++) {
			if (0 == strcmp(argv[i], "send") && argc >= 6) {
				uint8_t addr_to;
				uint8_t addr_from;
				uint8_t app_addr_to;
				uint8_t app_addr_from;
				char* endptr;
				char message[150];
				struct send_to_node_ret_payload* send_payload;

				*cmd = REQUEST_SEND;
				addr_to = UINT8_MAX;
				addr_from = UINT8_MAX;
				for (i = 0; i < argc; i++) {
					if (0 == strcmp(argv[i], "-s") || 0 == strcmp(argv[i], "--sender")) {
						endptr = NULL;
						addr_from = (uint8_t) strtol(argv[i + 1], &endptr, 10);
						if (argv[i + 1] == endptr) {
							return false;
						}
					}
					if (0 == strcmp(argv[i], "-r") || 0 == strcmp(argv[i], "--receiver")) {
						endptr = NULL;
						addr_to = (uint8_t) strtol(argv[i + 1], &endptr, 10);
						if (argv[i + 1] == endptr) {
							return false;
						}
					}
				}

				if (addr_to == UINT8_MAX || addr_from == UINT8_MAX) {
					custom_log_error("Failed to parse send command");
					return false;
				}

				*payload = malloc(sizeof(struct send_to_node_ret_payload));
				send_payload = (struct send_to_node_ret_payload*) *payload;

				send_payload->addr_from = addr_from;
				send_payload->addr_to = addr_to;

				if (argc > 6) {
					for (i = 0; i < argc; i++) {
						if (0 == strcmp(argv[i], "--app") || 0 == strcmp(argv[i], "-a")) {
							strcpy(message, argv[i + 1]);
						}
						if (0 == strcmp(argv[i], "-ar") || 0 == strcmp(argv[i], "--app-receiver")) {
							endptr = NULL;
							app_addr_to = (uint8_t) strtol(argv[i + 1], &endptr, 10);
							if (argv[i + 1] == endptr) {
								return false;
							}
						}
						if (0 == strcmp(argv[i], "-as") || 0 == strcmp(argv[i], "--app-sender")) {
							endptr = NULL;
							app_addr_from = (uint8_t) strtol(argv[i + 1], &endptr, 10);
							if (argv[i + 1] == endptr) {
								return false;
							}
						}

					}

					send_payload->app_payload.addr_from = app_addr_from;
					send_payload->app_payload.addr_to = app_addr_to;
					send_payload->app_payload.message_len = (uint8_t) strlen(message);
					memcpy(send_payload->app_payload.message, message, send_payload->app_payload.message_len);
				} else {
					send_payload->app_payload.addr_from = 0;
					send_payload->app_payload.addr_to = 0;
					send_payload->app_payload.message_len = 0;
				}
				send_payload->app_payload.req_type = APP_REQUEST_DELIVERY;

				return true;
			}

			if (0 == strcmp(argv[i], "ping")) {
				*cmd = REQUEST_PING;
				return create_addr_payload(argv[i + 1], payload);
			} else if (0 == strcmp(argv[i], "kill")) {
				*cmd = REQUEST_KILL_NODE;
				return create_addr_payload(argv[i + 1], payload);
			} else if (0 == strcmp(argv[i], "revive")) {
				*cmd = REQUEST_REVIVE_NODE;
				return create_addr_payload(argv[i + 1], payload);
			}
		}
	} else if (argc == 2) {
		if (0 == strcmp(argv[1], "reset")) {
			*cmd = REQUEST_RESET;
		}

		return true;
	}

	return false;
}
