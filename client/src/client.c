#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "custom_logger.h"
#include "control_utils.h"
#include "connection.h"
#include "format.h"

#ifndef SERVER_PORT
#define SERVER_PORT 999
#endif

static void parse_args(int32_t argc, char** argv, enum request_type* cmd);

int32_t main(int32_t argc, char** argv) {
	int32_t server_fd;
	enum request_type req;

	server_fd = connection_socket_to_send(SERVER_PORT);

	if (server_fd < 0) {
		die("Failed to get socket");
	}

	parse_args(argc, argv, &req);

	close(server_fd);

	return 0;
}

static void parse_args(int32_t argc, char** argv, enum request_type* cmd) {
	int32_t i;

	for (i = 0; i < argc; i++) {
		if (0 == strcmp(argv[i], "send")) {
			*cmd = REQUEST_TYPE_SEND_AS_NODE;
			break;
		}
		if (0 == strcmp(argv[i], "ping")) {
			*cmd = REQUEST_TYPE_PING_NODE;
			break;
		}
	}
}
