#include "node_handler.h"

#include "node_essentials.h"
#include "io.h"
#include "format_client_server.h"
#include "control_utils.h"

// if there is multiple clients then replace it with id key array and check it for occurence
// id then should be taken from server in order to be unique between nodes
static bool stop_broadcast = false;
static bool stop_inverse = false;
static bool was_message = false;

int32_t handle_ping(int32_t conn_fd) {
	enum request_result req_res;

	req_res = REQUEST_OK;
	if (io_write_all(conn_fd, (char*) &req_res, sizeof_enum(req_res))) {
		node_log_error("Failed to response to ping");
		return -1;
	}

	return 0;
}

int32_t handle_server_send(enum request cmd_type, int32_t label, const void* payload, const routing_table_t* routing) { // NOLINT
	struct send_to_node_ret_payload* ret_payload;
	uint8_t b[256];
	uint32_t buf_len;
	int32_t node_conn;
	int32_t next_label;
	int32_t res;

	res = 0;
	ret_payload = (struct send_to_node_ret_payload*) payload;
	if (ret_payload->label_to == label) {
		node_log_warn("Message for node itself");
		if (node_essentials_notify_server()) {
			node_log_error("Failed to notify");
			res = -1;
		}
		return res;
	}

	if (format_node_node_create_message(cmd_type, payload, b, &buf_len)) {
		node_log_error("Failed to create message");
		res = -1;
		return res;
	}

	node_log_debug("Finding route to %d", ret_payload->label_to);
	next_label = routing_next_label(routing, ret_payload->label_to);
	if (next_label < 0) {
		node_log_error("Failed to find route");

		struct node_route_payload route_payload = {
			.local_sender_label = label,
			.sender_label = ret_payload->label_from,
			.receiver_label = ret_payload->label_to,
			.metric = 0,
			.time_to_live = TTL
			// id is not used yet
		};

		node_essentials_broadcast(label, -1, &route_payload, stop_broadcast);

		res = -1;
		return res;
	}

	node_conn = node_essentials_get_conn(node_port(next_label));
	if (node_conn < 0) {
		node_log_error("Failed to create connection with node %d", next_label);
		res = -1;
	} else {
		if (io_write_all(node_conn, (char*) b, buf_len)) {
			node_log_error("Failed to send request");
			res = -1;
		}
	}

	return 0;
}

int32_t handle_node_send(int32_t label, const void* payload, const routing_table_t* routing) {
	int32_t label_to;
	int32_t res;

	node_log_warn("Send node %d", label);

	res = 0;
	label_to = ((struct node_send_payload*) payload)->label_to;
	if (label_to == label) {
		if (node_essentials_notify_server()) {
			node_log_error("Failed to notify");
			res = -1;
			return res;
		}
	} else {
		struct node_send_payload* ret_payload;
		int32_t node_conn;
		int32_t next_label;

		ret_payload = (struct node_send_payload*) payload;

		next_label = routing_next_label(routing, ret_payload->label_to);
		if (next_label < 0) {
			node_log_error("Failed to find path in table");
			// TODO: this may happen if node died after path was found
			// start broadcast from here
			res = -1;
			return res;
		}

		node_conn = node_essentials_get_conn(node_port(next_label));

		if (node_conn < 0) {
			node_log_error("Failed to create connection with node %d", ret_payload->label_to);
			// TODO: this may happen if next label from routing table is died
			// delete old label from routing and start broadcast from here
			return -1;
		} else {
			uint8_t b[256];
			uint32_t buf_len;

			if (format_node_node_create_message(REQUEST_SEND, ret_payload, b, &buf_len)) {
				node_log_error("Failed to create node-send message");
				return -1;
			}

			if (io_write_all(node_conn, (char*) b, buf_len)) {
				node_log_error("Failed to send request");
				res = -1;
			}
		}
	}

	return res;
}

int32_t handle_node_route_direct(routing_table_t* routing, int32_t server_label, void* payload) {
	struct node_route_payload* route_payload;
	int32_t prev_label;

	route_payload = (struct node_route_payload*) payload;

	if (was_message) {
		return 0;
	} else {
		was_message = true;
	}

	if (routing_next_label(routing, route_payload->sender_label) < 0) {
		routing_set_label(routing, route_payload->sender_label, route_payload->local_sender_label, route_payload->metric);
	} else {
		if (routing_get(routing, route_payload->sender_label).metric > route_payload->metric) {
			routing_set_label(routing, route_payload->sender_label, route_payload->local_sender_label, route_payload->metric);
		}
	}

	if (route_payload->receiver_label == server_label) {
		uint8_t b[256];
		uint32_t buf_len;
		int32_t next_label_to_back;
		int32_t conn_fd;

		node_log_info("Reached the recevier label %d", route_payload->receiver_label);

		if (!stop_inverse) {
			// replace with finding id function
			stop_inverse = true;
		} else {
			// somebody alreaady reached this label with this message id and coming back
			return 0;
		}

		node_essentials_notify_server();

		route_payload->metric = 0;
		route_payload->time_to_live = TTL;
		route_payload->local_sender_label = server_label;

		if (format_node_node_create_message(REQUEST_ROUTE_INVERSE, route_payload, b, &buf_len)) {
			node_log_error("Failed to create route inverse message");
			return -1;
		}

		next_label_to_back = routing_next_label(routing, route_payload->sender_label);
		if (next_label_to_back < 0) {
			node_log_error("Failed to get next label to %d", route_payload->sender_label);
			return -1;
		}

		conn_fd = node_essentials_get_conn(node_port(next_label_to_back));

		if (conn_fd < 0) {
			node_log_error("Failed to connect to %d node", next_label_to_back);
			routing_del(routing, route_payload->sender_label);
			return -1;
		}

		if (io_write_all(conn_fd, (char*) b, buf_len)) {
			node_log_error("Failed to send route inverse request");
			return -1;
		}

		return 0;
	}

	if (route_payload->time_to_live <= 0) {
		node_log_warn("Message died with ttl %d", route_payload->time_to_live);
		return 0;
	}

	prev_label = route_payload->local_sender_label;
	route_payload->local_sender_label = server_label;

	node_essentials_broadcast(server_label, prev_label, route_payload, stop_broadcast);

	return 0;
}

int32_t handle_node_route_inverse(routing_table_t* routing, void* payload, int32_t server_label) {
	struct node_route_payload* route_payload;
	int32_t conn_fd;
	int32_t next_label;
	uint8_t b[256];
	uint32_t buf_len;

	route_payload = (struct node_route_payload*) payload;

	node_log_warn("Inverse node %d", server_label);

	if (routing_next_label(routing, route_payload->receiver_label) < 0) {
		routing_set_label(routing, route_payload->receiver_label, route_payload->local_sender_label, route_payload->metric);
	}

	if (route_payload->sender_label == server_label) {
		node_log_info("Route inverse request came back");
		return 0;
	}

	next_label = routing_next_label(routing, route_payload->sender_label);
	if (next_label < 0) {
		node_log_error("Failed to get next label to %d", route_payload->sender_label);
		return -1;
	}

	route_payload->local_sender_label = server_label;
	route_payload->metric++;

	if (format_node_node_create_message(REQUEST_ROUTE_INVERSE, route_payload, b, &buf_len)) {
		node_log_error("Failed to create route inverse message");
		return -1;
	}

	conn_fd = node_essentials_get_conn(node_port(next_label));

	if (conn_fd < 0) {
		// probably notify client that message is delivered after all but log error
		// on next send there should be route direct from this node and path will be rebuilt
		node_log_error("Failed to connect to node %d while travel back", next_label);
		routing_del(routing, route_payload->sender_label);
		node_essentials_notify_server();
		return -1;
	}

	if (io_write_all(conn_fd, (char*) b, buf_len)) {
		node_log_error("Failed to send route inverse request");
		return -1;
	}

	return 0;
}

void handle_stop_broadcast(void) {
	stop_broadcast = true;
}

void handle_reset_broadcast_status(void) {
	stop_broadcast = false;
	stop_inverse = false;
	was_message = false;
}
