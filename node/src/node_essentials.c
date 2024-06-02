#include "node_essentials.h"

#include <unistd.h>

#include "connection.h"
#include "io.h"
#include "format_server_node.h"

struct conn {
	int32_t fd;
	uint16_t port;
};

// server also counted
#define CONNECTIONS 29
static struct conn connections[CONNECTIONS];

static uint16_t broadcast_neighbors[CONNECTIONS];
static uint8_t neighbor_num = 0;

void node_essentials_reset_connections(void) {
	for (size_t i = 0; i < CONNECTIONS; i++) {
		if (connections[i].port != SERVER_PORT) {
			close(connections[i].fd);
			connections[i].fd = -1;
			connections[i].port = UINT16_MAX;
		}
	}
}

int32_t node_essentials_get_conn(uint16_t port) {
	static bool init = false;
	size_t i;

	if (!init) {
		for (i = 0; i < CONNECTIONS; i++) {
			connections[i].fd = -1;
			connections[i].port = UINT16_MAX;
		}
		init = true;
	}

	for (i = 0; i < CONNECTIONS; i++) {
		if (connections[i].fd > 0 && connections[i].port == port) {
			return connections[i].fd;
		}
	}

	for (i = 0; i < CONNECTIONS; i++) {
		if (connections[i].fd == -1) {
			connections[i].fd = connection_socket_to_send(port);
			if (connections[i].fd < 0) {
				/* node_log_error("Failed to open connection with %d", port); */
				break;
			}
			connections[i].port = port;

			return connections[i].fd;
		}
	}

	/* node_log_error("Failed to open connection with %d", port); */

	return -1;
}

bool node_essentials_notify_server(notify_t* notify) {
	uint8_t b[NOTIFY_LEN + MSG_LEN];
	msg_len_type buf_len;
	int32_t server_fd;

	format_server_node_create_message(REQUEST_NOTIFY, notify, b, &buf_len);

	server_fd = node_essentials_get_conn(SERVER_PORT);
	if (server_fd < 0) {
		node_log_error("Failed to connect to server");
		return false;
	} else {
		if (!io_write_all(server_fd, b, buf_len)) {
			node_log_error("Failed to send notify request to server");
			return false;
		}
	}

	return true;
}

static void get_conn_and_send(uint16_t port, uint8_t* buf, msg_len_type buf_len);

void node_essentials_broadcast_route(struct node_route_direct_payload* route_payload, bool stop_broadcast) {
	uint8_t b[MAX_MSG_LEN];
	msg_len_type buf_len;
	size_t i;

	if (!stop_broadcast) {

		route_payload->time_to_live--;
		route_payload->metric++;

		format_node_node_create_message(REQUEST_ROUTE_DIRECT, route_payload, b, &buf_len);

		for (i = 0; i < neighbor_num; i++) {
			get_conn_and_send(broadcast_neighbors[i], b, buf_len);
		}
	}
}

void node_essentials_broadcast(broadcast_t* broadcast_payload) {
	uint8_t b[MAX_MSG_LEN];
	msg_len_type buf_len;
	size_t i;

	for (i = 0; i < neighbor_num; i++) {
		send_t send_payload = {
			.app_payload = broadcast_payload->app_payload,
			.addr_from = broadcast_payload->addr_from,
			.addr_to = (uint8_t) node_addr(broadcast_neighbors[i]),
		};

		format_node_node_create_message(REQUEST_SEND, &send_payload, b, &buf_len);
		get_conn_and_send(broadcast_neighbors[i], b, buf_len);
	}
}

static inline uint8_t from_pos(const int8_t pos[2]) {
	return (uint8_t) (pos[0] * MATRIX_SIZE + pos[1]);
}

static void fill_broadcast_neighbors(uint8_t addr, uint8_t radius);

void node_essentials_fill_neighbors_port(uint8_t addr) {
	fill_broadcast_neighbors(addr, BROADCAST_RADIUS);
}

static void left_right_neighbours(int8_t col, uint8_t radius, int8_t i, int8_t row) {
	int8_t j;

	j = (int8_t) (col - 1);
	if (i != row - radius && i != row + radius) {
		for (; j > (col - radius) && j >= 0; j--) {
			broadcast_neighbors[neighbor_num++] = node_port(from_pos((int8_t[]) { i, j }));
		}
	}
	j = (int8_t) (col + 1);
	if (i != row - radius && i != row + radius) {
		for (; j < (col + radius) && j < MATRIX_SIZE; j++) {
			broadcast_neighbors[neighbor_num++] = node_port(from_pos((int8_t[]) { i, j }));
		}
	}
}

static void fill_broadcast_neighbors(uint8_t addr, uint8_t radius) { // NOLINT
	int8_t i;
	int8_t j;
	int8_t row;
	int8_t col;

	row = (int8_t) (addr / MATRIX_SIZE);
	col = (int8_t) (addr % MATRIX_SIZE);


	if (row < 0 || row >= MATRIX_SIZE || col < 0 || col >= MATRIX_SIZE) {
        return;
    }

	// up
	for (i = (int8_t) (row - 1); i >= row - radius && i>= 0; i--) {
		if (i >= 0) {
			j = col;
			broadcast_neighbors[neighbor_num++] = node_port(from_pos((int8_t[]) { i, j }));
			left_right_neighbours(col, radius, i, row);
		}
	}
	// down
	for (i = (int8_t) (row + 1); i <= row + radius && i < MATRIX_SIZE; i++) {
		if (i < MATRIX_SIZE) {
			j = col;
			broadcast_neighbors[neighbor_num++] = node_port(from_pos((int8_t[]) { i, j }));
			left_right_neighbours(col, radius, i, row);
		}
	}

	//left
	i = row;
	for (j = (int8_t) (col - 1); j >= 0 && j >= col - radius; j--) {
		broadcast_neighbors[neighbor_num++] = node_port(from_pos((int8_t[]) { i, j }));

	}
	//right
	for (j = (int8_t) (col + 1); j < MATRIX_SIZE && j <= col + radius; j++) {
		broadcast_neighbors[neighbor_num++] = node_port(from_pos((int8_t[]) { i, j }));
	}
}

static void get_conn_and_send(uint16_t port, uint8_t* buf, msg_len_type buf_len) {
	int32_t conn_fd;

	conn_fd = node_essentials_get_conn(port);

	if (conn_fd < 0) {
		/* node_log_error("Failed to open connection with neighbor port %d", port); */
		return;
	}

	if (!io_write_all(conn_fd, buf, buf_len)) {
		node_log_error("Failed to send route direct request: address %d", node_addr(port));
		return;
	}
}
