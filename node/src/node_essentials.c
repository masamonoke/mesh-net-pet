#include "node_essentials.h"

#include <unistd.h>

#include "connection.h"
#include "io.h"

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

static int32_t get_conn(uint16_t port) {
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
				break;
			}
			connections[i].port = port;

			return connections[i].fd;
		}
	}

	return -1;
}

bool node_essentials_notify_server(notify_t* notify) {
	uint8_t b[sizeof(notify_t) + MSG_BASE_LEN];
	msg_len_type buf_len;
	int32_t server_fd;

	format_create(REQUEST_NOTIFY, notify, b, &buf_len, REQUEST_SENDER_NODE);

	server_fd = get_conn(SERVER_PORT);
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

void node_essentials_broadcast_route(node_packet_t* route_payload, bool stop_broadcast) {
	uint8_t b[MAX_MSG_LEN];
	msg_len_type buf_len;
	size_t i;

	if (!stop_broadcast) {

		route_payload->time_to_live--;

		format_create(REQUEST_ROUTE_DIRECT, route_payload, b, &buf_len, REQUEST_SENDER_NODE);

		for (i = 0; i < neighbor_num; i++) {
			node_essentials_get_conn_and_send(broadcast_neighbors[i], b, buf_len);
		}
	}
}

void node_essentials_broadcast(node_packet_t* broadcast_payload) {
	uint8_t b[MAX_MSG_LEN];
	msg_len_type buf_len;
	size_t i;

	for (i = 0; i < neighbor_num; i++) {
		broadcast_payload->receiver_addr = (uint8_t) node_addr(broadcast_neighbors[i]);

		format_create(REQUEST_SEND, broadcast_payload, b, &buf_len, REQUEST_SENDER_NODE);

		node_essentials_get_conn_and_send(broadcast_neighbors[i], b, buf_len);
	}
}

void node_essentials_send_unicast_contest(unicast_contest_t* unicast) {
	uint8_t i;
	uint8_t b[sizeof(unicast_contest_t) + MSG_BASE_LEN];
	uint8_t buf_len;

	format_create(REQUEST_UNICAST_CONTEST, unicast, b, &buf_len, REQUEST_SENDER_NODE);
	for (i = 0; i < neighbor_num; i++) {
		node_essentials_get_conn_and_send(broadcast_neighbors[i], b, buf_len);
	}
}

void node_essentials_send_unicast_first(unicast_contest_t* unicast, uint8_t addr) {
	uint8_t b[sizeof(unicast_contest_t) + MSG_BASE_LEN];
	uint8_t buf_len;
	uint8_t prev_addr;

	prev_addr = unicast->node_addr;
	unicast->node_addr = addr;
	format_create(REQUEST_UNICAST_FIRST, unicast, b, &buf_len, REQUEST_SENDER_NODE);
	node_essentials_get_conn_and_send(node_port(prev_addr), b, buf_len);
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

bool node_essentials_get_conn_and_send(uint16_t port, uint8_t* buf, msg_len_type buf_len) {
	int32_t conn_fd;

	conn_fd = get_conn(port);

	if (conn_fd < 0) {
		return false;
	}

	if (!io_write_all(conn_fd, buf, buf_len)) {
		node_log_error("Failed to send route direct request: address %d", node_addr(port));
		return false;
	}

	return true;
}
