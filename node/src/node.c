#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <uuid/uuid.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "custom_logger.h"
#include "control_utils.h"
#include "connection.h"
#include "format.h"
#include "io.h"

static char* node_name;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#define node_log_info(format, ...) custom_log_info(ANSI_COLOR_MAGENTA "Node " ANSI_COLOR_RESET"%s: " format, node_name, ##__VA_ARGS__)
#define node_log_error(format, ...) custom_log_error(ANSI_COLOR_MAGENTA "Node " ANSI_COLOR_RESET"%s: " format, node_name, ##__VA_ARGS__)
#define node_log_warn(format, ...) custom_log_warn(ANSI_COLOR_MAGENTA "Node " ANSI_COLOR_RESET"%s: " format, node_name, ##__VA_ARGS__)
#define node_log_debug(format, ...) custom_log_debug(ANSI_COLOR_MAGENTA "Node " ANSI_COLOR_RESET"%s: " format, node_name, ##__VA_ARGS__)

#pragma clang diagnostic pop

#endif

static volatile bool keeprunning = true;

static void int_handler(int32_t dummy);

static void parse_args(char** args, size_t argc, char** alias, int32_t* node_label, int32_t* port);

int32_t main(int32_t argc, char** argv) {
	int32_t label;
	int32_t port;
	int32_t server_fd;
	uint8_t buf[256];
	uint32_t buf_len;

	signal(SIGINT, int_handler);

	server_fd = connection_socket_to_send();

	if (server_fd < 0) {
		die("Failed to get socket");
	}

	parse_args(argv, (size_t) argc, &node_name, &label, &port);

	node_log_info("Node started on port %d", port);

	struct node_upate_ret_payload payload = {
		.label = label,
		.pid = getpid(),
		.port = port
	};
	strcpy(payload.alias, node_name);
	format_create_message(REQUEST_TYPE_NODE_UPDATE, &payload, buf, &buf_len);
	if (io_write_all(server_fd, (const char*) buf, buf_len)) {
		node_log_error("Failed to send node init data");
	}

	while (keeprunning) {
	}

	close(server_fd);
	node_log_info("Shutting down node pid = %d", getpid());

	return 0;
}

static void int_handler(int32_t dummy) {
	(void) dummy;
	keeprunning = false;
}


static void parse_args(char** args, size_t argc, char** alias, int32_t* node_label, int32_t* port) { // NOLINT
	size_t i;

	if (argc < 2) {
		die("Wrong number of arguments");
	}

	for (i = 0; i < argc - 1; i++) {
		if (0 == strcmp("-alias", args[i])) {
			*alias = args[i + 1];
		}

		if (0 == strcmp("-nodelabel", args[i])) {
			*node_label = (int32_t) strtol(args[i + 1], NULL, 10);
		}

		if (0 == strcmp("-port", args[i])) {
			*port = (int32_t) strtol(args[i + 1], NULL, 10);
		}
	}
}
