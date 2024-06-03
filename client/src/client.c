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
#include "io.h"
#include "settings.h"
#include "crc.h"

__attribute__((warn_unused_result))
static bool parse_args(int32_t argc, char** argv, enum request* cmd, void** payload);

int32_t main(int32_t argc, char** argv) {
	int32_t server_fd;
	enum request req;
	uint8_t buf[MAX_MSG_LEN];
	msg_len_type buf_len;
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
	format_create(req, payload, buf, &buf_len, REQUEST_SENDER_CLIENT);

	if (!io_write_all(server_fd, buf, buf_len)) {
		custom_log_error("Failed to send client command");
	}

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	status = REQUEST_UNKNOWN;

	// recv is used for timeout
	received_bytes = recv(server_fd, buf, sizeof(buf), 0);
	if (received_bytes > 0) {
		enum_ir tmp;

		memcpy(&tmp, buf, sizeof_enum(status));
		status = (enum request_result) tmp;
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

static bool parse_send_cmd(int32_t argc, char** argv, enum request* cmd, void** payload);

static bool parse_broadcast_cmd(int32_t argc, char** argv, void** payload, enum app_request app_req);

static bool parse_args(int32_t argc, char** argv, enum request* cmd, void** payload) {
	int32_t i;

	if (argc > 2) {
		for (i = 0; i < argc; i++) {
			if (0 == strcmp(argv[i], "send") && argc >= 6) {
				return parse_send_cmd(argc, argv, cmd, payload);
			}

			if (0 == strcmp(argv[i], "broadcast") && argc >= 4) {
				*cmd = REQUEST_BROADCAST;
				return parse_broadcast_cmd(argc, argv, payload, APP_REQUEST_BROADCAST);
			}

			if (0 == strcmp(argv[i], "unicast") && argc >= 4) {
				*cmd = REQUEST_UNICAST;
				return parse_broadcast_cmd(argc, argv, payload, APP_REQUEST_UNICAST);
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

static bool parse_send_cmd(int32_t argc, char** argv, enum request* cmd, void** payload) {
	uint8_t addr_to;
	uint8_t addr_from;
	uint8_t app_addr_to;
	uint8_t app_addr_from;
	char* endptr;
	char message[APP_MESSAGE_LEN];
	node_packet_t* send_payload;
	int32_t i;

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

	*payload = malloc(sizeof(node_packet_t));
	send_payload = (node_packet_t*) *payload;

	send_payload->sender_addr = addr_from;
	send_payload->receiver_addr = addr_to;

	if (argc > 6) {
		for (i = 0; i < argc; i++) {
			if (0 == strcmp(argv[i], "--app") || 0 == strcmp(argv[i], "-a")) {
				if (strlen(argv[i + 1]) > APP_MESSAGE_LEN - 1) {
					custom_log_warn("You passed message > 150 symbols. It will be trimmed to 150 symbols.");
					strncpy(message, argv[i + 1], APP_MESSAGE_LEN - 1);
				} else {
					strcpy(message, argv[i + 1]);
				}
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
	send_payload->crc = packet_crc(send_payload);
	return true;
}

static bool parse_broadcast_cmd(int32_t argc, char** argv, void** payload, enum app_request app_req) {
	uint8_t addr_from;
	char* endptr;
	char message[APP_MESSAGE_LEN];
	node_packet_t* broadcast_payload;
	int32_t i;

	addr_from = UINT8_MAX;
	for (i = 0; i < argc; i++) {
		if (0 == strcmp(argv[i], "-s") || 0 == strcmp(argv[i], "--sender")) {
			endptr = NULL;
			addr_from = (uint8_t) strtol(argv[i + 1], &endptr, 10);
			if (argv[i + 1] == endptr) {
				return false;
			}
		}
	}

	if (addr_from == UINT8_MAX) {
		custom_log_error("Failed to parse send command");
		return false;
	}

	*payload = malloc(sizeof(node_packet_t));
	broadcast_payload = (node_packet_t*) *payload;

	broadcast_payload->sender_addr = addr_from;

	if (argc >= 6) {
		for (i = 0; i < argc; i++) {
			if (0 == strcmp(argv[i], "--app") || 0 == strcmp(argv[i], "-a")) {
				if (strlen(argv[i + 1]) > APP_MESSAGE_LEN - 1) {
					custom_log_warn("You passed message > 150 symbols. It will be trimmed to 150 symbols.");
					strncpy(message, argv[i + 1], APP_MESSAGE_LEN - 1);
				} else {
					strcpy(message, argv[i + 1]);
				}
			}
		}

		broadcast_payload->app_payload.message_len = (uint8_t) strlen(message);
		memcpy(broadcast_payload->app_payload.message, message, broadcast_payload->app_payload.message_len);
	} else {
		broadcast_payload->app_payload.addr_from = 0;
		broadcast_payload->app_payload.addr_to = 0;
		broadcast_payload->app_payload.message_len = 0;
	}

	broadcast_payload->app_payload.req_type = app_req;
	broadcast_payload->time_to_live = BROADCAST_RADIUS;
	broadcast_payload->app_payload.crc = crc16(broadcast_payload->app_payload.message, broadcast_payload->app_payload.message_len);

	return true;
}
