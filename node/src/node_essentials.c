#include "node_essentials.h"

#include <unistd.h>

#include "node_server.h"
#include "connection.h"
#include "io.h"
#include "format_server_node.h"

struct conn {
	int32_t fd;
	int32_t port;
};

#define CONNECTIONS 5
static struct conn connections[CONNECTIONS];

void node_essentials_reset_connections(void) {
	for (size_t i = 0; i < CONNECTIONS; i++) {
		if (connections[i].port != SERVER_PORT) {
			close(connections[i].fd);
			connections[i].fd = -1;
			connections[i].port = -1;
		}
	}
}

int32_t node_essentials_get_conn(int32_t port) {
	static bool init = false;
	size_t i;

	if (!init) {
		for (i = 0; i < CONNECTIONS; i++) {
			connections[i].fd = -1;
			connections[i].port = -1;
		}
		init = true;
	}

	for (i = 0; i < CONNECTIONS; i++) {
		if (connections[i].port == port) {
			return connections[i].fd;
		}
	}

	for (i = 0; i < CONNECTIONS; i++) {
		if (connections[i].fd == -1) {
			connections[i].fd = connection_socket_to_send((uint16_t) port);
			if (connections[i].fd < 0) {
				node_log_error("Failed to open connection with %d", port);
				break;
			}
			connections[i].port = port;

			return connections[i].fd;
		}
	}

	node_log_error("Failed to open connection with %d", port);

	return -1;
}

int32_t node_essentials_notify_server(void) {
	uint8_t b[256];
	uint32_t buf_len;
	int32_t server_fd;
	struct node_notify_payload notify_payload = {
		.notify_type = NOTIFY_GOT_MESSAGE
	};

	node_log_info("Got message");

	if (format_server_node_create_message(REQUEST_NOTIFY, (void*) &notify_payload, b, &buf_len)) {
		node_log_error("Failed to create notify message");
		return -1;
	}

	server_fd = node_essentials_get_conn(SERVER_PORT);
	if (server_fd < 0) {
		node_log_error("Failed to connect to server");
		return -1;
	} else {
		if (io_write_all(server_fd, (char*) b, buf_len)) {
			node_log_error("Failed to send notify request to server");
			return -1;
		}
	}

	return 0;
}

static void fill_neighbour_port(int32_t label, int32_t* up_port, int32_t* down_port, int32_t* left_port, int32_t* right_port);

static int32_t get_conn_and_send(uint16_t port, char* buf, uint32_t buf_len);

int32_t node_essentials_broadcast(int32_t current_label, int32_t banned_label, struct node_route_payload* route_payload, bool stop_broadcast) { // NOLINT
	int32_t up_port;
	int32_t down_port;
	int32_t left_port;
	int32_t right_port;
	uint8_t b[256];
	uint32_t buf_len;
	int32_t banned_port;

	if (!stop_broadcast) {

		route_payload->time_to_live--;
		route_payload->metric++;

		fill_neighbour_port(current_label, &up_port, &down_port, &left_port, &right_port);

		if (banned_label > 0) {
			banned_port = node_port(banned_label);
		} else {
			banned_port = -1;
		}

		if (format_node_node_create_message(REQUEST_ROUTE_DIRECT, route_payload, b, &buf_len)) {
			node_log_error("Failed to create route direct message");
			return -1;
		}

		if (up_port > 0 && up_port != banned_port) {
			get_conn_and_send((uint16_t) up_port, (char*) b, buf_len);
		}

		if (down_port > 0 && down_port != banned_port) {
			get_conn_and_send((uint16_t) down_port, (char*) b, buf_len);
		}

		if (left_port > 0 && left_port != banned_port) {
			get_conn_and_send((uint16_t) left_port, (char*) b, buf_len);
		}

		if (right_port > 0 && right_port != banned_port) {
			get_conn_and_send((uint16_t) right_port, (char*) b, buf_len);
		}
	}

	return 0;
}

static inline bool is_valid_pos(const ssize_t pos[2]) {
	return ((pos[0] >= 0 && pos[0] < MATRIX_SIZE) && (pos[1] >= 0 && pos[1] < MATRIX_SIZE));
}

static inline int32_t from_pos(const ssize_t pos[2]) {
	return (int32_t) (pos[0] * MATRIX_SIZE + pos[1]);
}

static void fill_neighbour_port(int32_t label, int32_t* up_port, int32_t* down_port, int32_t* left_port, int32_t* right_port) {
	ssize_t i;
	ssize_t j;
	ssize_t up_pos[2];
	ssize_t down_pos[2];
	ssize_t left_pos[2];
	ssize_t right_pos[2];

	// labels placed in matrix in sequential order starting from 0
	i = label / MATRIX_SIZE;
	j = label % MATRIX_SIZE;

	up_pos[0] = i - 1, up_pos[1] = j;
	down_pos[0] = i + 1, down_pos[1] = j;
	left_pos[0] = i, left_pos[1] = j - 1;
	right_pos[0] = i, right_pos[1] = j + 1;

	*up_port = -1, *down_port = -1, *left_port = -1, *right_port = -1;

	if (is_valid_pos(up_pos)) {
		*up_port = node_port(from_pos(up_pos));
	}

	if (is_valid_pos(down_pos)) {
		*down_port = node_port(from_pos(down_pos));
	}

	if (is_valid_pos(left_pos)) {
		*left_port = node_port(from_pos(left_pos));
	}

	if (is_valid_pos(right_pos)) {
		*right_port = node_port(from_pos(right_pos));
	}
}

static int32_t get_conn_and_send(uint16_t port, char* buf, uint32_t buf_len) {
	int32_t conn_fd;

	conn_fd = node_essentials_get_conn(port);

	if (conn_fd < 0) {
		node_log_error("Failed to open connection with neighbor port %d", port);
		return -1;
	}

	if (io_write_all(conn_fd, buf, buf_len)) {
		node_log_error("Failed to send route direct request: label %d", node_label(port));
		return -1;
	}

	return 0;
}
