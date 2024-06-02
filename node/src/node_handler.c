#include "node_handler.h"

#include <memory.h>

#include "node_essentials.h"
#include "io.h"
#include "node_app.h"

#include <assert.h>

// if there is multiple clients then replace it with id key array and check it for occurence
// id then should be taken from server in order to be unique between nodes
static bool stop_broadcast = false;
static bool stop_inverse = false;
static bool was_message = false;

bool handle_ping(int32_t conn_fd) {
	enum request_result req_res;

	req_res = REQUEST_OK;
	if (!io_write_all(conn_fd, (uint8_t*) &req_res, sizeof_enum(req_res))) {
		node_log_error("Failed to response to ping");
		return false;
	}

	return true;
}

bool handle_server_send(enum request cmd_type, uint8_t addr, const void* payload, const routing_table_t* routing, app_t apps[APPS_COUNT]) { // NOLINT
	send_t* ret_payload;
	uint8_t b[MAX_MSG_LEN];
	msg_len_type buf_len;
	int32_t node_conn;
	uint8_t next_addr;
	bool res;

	res = true;
	ret_payload = (send_t*) payload;

	if (ret_payload->addr_to == addr) {
		node_log_warn("Message for node itself");

		if (node_app_handle_request(apps, &ret_payload->app_payload)) {
			if (!node_essentials_notify_server(NOTIFY_GOT_MESSAGE)) {
				node_log_error("Failed to notify server");
			}
		} else {
			node_log_error("Failed to handle app request");
			if (!node_essentials_notify_server(NOTIFY_FAIL)) {
				node_log_error("Failed to notify fail");
			}
		}

		return res;
	}

	node_log_debug("Finding route to %d", ret_payload->addr_to);
	next_addr = routing_next_addr(routing, ret_payload->addr_to);
	if (next_addr == UINT8_MAX) {
		node_log_warn("Failed to find route");

		struct node_route_direct_payload route_payload = {
			.local_sender_addr = addr,
			.sender_addr = ret_payload->addr_from,
			.receiver_addr = ret_payload->addr_to,
			.metric = 0,
			.time_to_live = TTL,
			.app_payload = ret_payload->app_payload
			// id is not used yet
		};

		node_essentials_broadcast_route(&route_payload, stop_broadcast);

		return false;
	}

	node_app_setup_delivery(&ret_payload->app_payload);

	format_node_node_create_message(cmd_type, ret_payload, b, &buf_len);

	node_conn = node_essentials_get_conn(node_port(next_addr));
	if (node_conn < 0) {
		node_log_error("Failed to create connection with node %d", next_addr);
		res = false;
	} else {
		if (!io_write_all(node_conn, b, buf_len)) {
			node_log_error("Failed to send request");
			res = false;
		} else {
			node_log_info("Sent message (length %d) from %d:%d to %d:%d",
				ret_payload->app_payload.message_len, ret_payload->addr_from, ret_payload->app_payload.addr_from, ret_payload->addr_to, ret_payload->app_payload.addr_to);
		}
	}

	return res;
}

void handle_broadcast(broadcast_t* broadcast_payload) {
	node_app_setup_delivery(&broadcast_payload->app_payload);
	node_essentials_broadcast(broadcast_payload);
}

__attribute__((warn_unused_result))
static bool node_handle_app_request(app_t apps[APPS_COUNT], send_t* send_payload);

__attribute__((warn_unused_result))
static bool send_next(const routing_table_t* routing, send_t* ret_payload);

bool handle_node_send(uint8_t addr, const void* payload, const routing_table_t* routing, app_t apps[APPS_COUNT]) {
	uint8_t addr_to;
	bool res;

	node_log_warn("Send node %d", addr);

	res = true;
	addr_to = ((send_t*) payload)->addr_to;
	if (addr_to == addr) {
		if (!node_handle_app_request(apps, (send_t*) payload)) {
			node_log_error("Failed to handle app request");
			res = false;
		}
	} else {
		if (!send_next(routing, (send_t*) payload)) {
			node_log_error("Failed to send app request next");
			res = false;
		}
	}

	return res;
}

bool route_direct_handle_delivered(routing_table_t* routing, struct node_route_direct_payload* route_payload, uint8_t server_addr, app_t apps[APPS_COUNT]);

bool handle_node_route_direct(routing_table_t* routing, uint8_t server_addr, void* payload, app_t apps[APPS_COUNT]) {
	struct node_route_direct_payload* route_payload;

	route_payload = (struct node_route_direct_payload*) payload;

	if (was_message) {
		return 0;
	} else {
		was_message = true;
	}

	if (routing_next_addr(routing, route_payload->sender_addr) == UINT8_MAX) {
		routing_set_addr(routing, route_payload->sender_addr, route_payload->local_sender_addr, route_payload->metric);
	} else {
		if (routing_get(routing, route_payload->sender_addr).metric > route_payload->metric) {
			routing_set_addr(routing, route_payload->sender_addr, route_payload->local_sender_addr, route_payload->metric);
		}
	}

	if (route_payload->receiver_addr == server_addr) {
		route_direct_handle_delivered(routing, route_payload, server_addr, apps);
		return true;
	}

	if (route_payload->time_to_live <= 0) {
		node_log_warn("Message died with ttl %d", route_payload->time_to_live);
		return true;
	}

	route_payload->local_sender_addr = server_addr;

	node_essentials_broadcast_route(route_payload, stop_broadcast);

	return true;
}

bool handle_node_route_inverse(routing_table_t* routing, void* payload, uint8_t server_addr) {
	struct node_route_inverse_payload* route_payload;
	int32_t conn_fd;
	uint8_t next_addr;
	uint8_t b[ROUTE_INVERSE_LEN + MSG_LEN];
	msg_len_type buf_len;

	route_payload = (struct node_route_inverse_payload*) payload;

	node_log_warn("Inverse node %d", server_addr);

	if (routing_next_addr(routing, route_payload->receiver_addr) == UINT8_MAX) {
		routing_set_addr(routing, route_payload->receiver_addr, route_payload->local_sender_addr, route_payload->metric);
	}

	if (route_payload->sender_addr == server_addr) {
		node_log_debug("Route inverse request came back");
		return true;
	}

	next_addr = routing_next_addr(routing, route_payload->sender_addr);
	if (next_addr == UINT8_MAX) {
		node_log_error("Failed to get next addr to %d", route_payload->sender_addr);
		return false;
	}

	route_payload->local_sender_addr = server_addr;
	route_payload->metric++;

	format_node_node_create_message(REQUEST_ROUTE_INVERSE, route_payload, b, &buf_len);

	conn_fd = node_essentials_get_conn(node_port(next_addr));

	if (conn_fd < 0) {
		// probably notify client that message is delivered after all but log error
		// on next send there should be route direct from this node and path will be rebuilt
		node_log_error("Failed to connect to node %d while travel back", next_addr);
		routing_del(routing, route_payload->sender_addr);
		if (!node_essentials_notify_server(NOTIFY_INVERES_COMPLETED)) {
			node_log_error("Failed to notify server");
		}
		return false;
	}

	if (!io_write_all(conn_fd, b, buf_len)) {
		node_log_error("Failed to send route inverse request");
		return false;
	}

	return true;
}

void handle_stop_broadcast(void) {
	stop_broadcast = true;
}

void handle_reset_broadcast_status(void) {
	stop_broadcast = false;
	stop_inverse = false;
	was_message = false;
}

__attribute__((warn_unused_result))
static bool node_handle_app_request(app_t apps[APPS_COUNT], send_t* send_payload) {
	enum app_request app_req;
	bool res;

	app_req = send_payload->app_payload.req_type;
	res = true;

	if (app_req == APP_REQUEST_UNICAST) {
		if (stop_broadcast) {
			return true;;
		} else {
			if (!node_essentials_notify_server(NOTIFY_GOT_MESSAGE)) {
				node_log_error("Failed to notify server");
			}
			if (!node_essentials_notify_server(NOTIFY_UNICAST_HANDLED)) {
				node_log_error("Failed to notify server");
			}
		}
	}
	if (node_app_handle_request(apps, &send_payload->app_payload)) {
		if (!node_essentials_notify_server(NOTIFY_GOT_MESSAGE)) {
			node_log_error("Failed to notify server");
			res = false;
		}
	} else {
		if (!node_essentials_notify_server(NOTIFY_FAIL)) {
			node_log_error("Failed to notify fail");
		}
		res = false;
	}

	return res;
}

__attribute__((warn_unused_result))
static bool send_next(const routing_table_t* routing, send_t* ret_payload) {
	int32_t node_conn;
	uint8_t next_addr;

	next_addr = routing_next_addr(routing, ret_payload->addr_to);
	if (next_addr == UINT8_MAX) {
		node_log_error("Failed to find path in table");
		// TODO: this may happen if node died after path was found
		// start broadcast from here
		return false;
	}

	node_conn = node_essentials_get_conn(node_port(next_addr));

	if (node_conn < 0) {
		node_log_error("Failed to create connection with node %d", ret_payload->addr_to);
		// TODO: this may happen if next addr from routing table is died
		// delete old addr from routing and start broadcast from here
		return false;
	} else {
		uint8_t b[MAX_MSG_LEN];
		msg_len_type buf_len;

		format_node_node_create_message(REQUEST_SEND, ret_payload, b, &buf_len);

		if (!io_write_all(node_conn, b, buf_len)) {
			node_log_error("Failed to send request");
			return false;
		}
	}

	return true;
}

bool route_direct_handle_delivered(routing_table_t* routing, struct node_route_direct_payload* route_payload, uint8_t server_addr, app_t apps[APPS_COUNT]) {
	uint8_t b[MAX_MSG_LEN];
	msg_len_type buf_len;
	uint8_t next_addr_to_back;
	int32_t conn_fd;

	node_log_debug("Reached the recevier addr %d", route_payload->receiver_addr);

	if (!stop_inverse) {
		// replace with finding id function
		stop_inverse = true;
	} else {
		// somebody alreaady reached this addr with this message id and coming back
		return true;
	}

	route_payload->metric = 0;
	route_payload->time_to_live = TTL;
	route_payload->local_sender_addr = server_addr;

	format_node_node_create_message(REQUEST_ROUTE_INVERSE, route_payload, b, &buf_len);

	next_addr_to_back = routing_next_addr(routing, route_payload->sender_addr);
	if (next_addr_to_back == UINT8_MAX) {
		node_log_error("Failed to get next addr to %d", route_payload->sender_addr);
		return false;
	}

	conn_fd = node_essentials_get_conn(node_port(next_addr_to_back));

	if (conn_fd < 0) {
		node_log_error("Failed to connect to %d node", next_addr_to_back);
		routing_del(routing, route_payload->sender_addr);
		return false;
	}

	if (!io_write_all(conn_fd, b, buf_len)) {
		node_log_error("Failed to send route inverse request");
		return false;
	}

	if (node_app_handle_request(apps, &route_payload->app_payload)) {
		if (!node_essentials_notify_server(NOTIFY_GOT_MESSAGE)) {
			node_log_error("Failed to notify server");
		}
	} else {
		node_log_error("Failed to handle app request");
		if (!node_essentials_notify_server(NOTIFY_FAIL)) {
			node_log_error("Failed to notify fail");
		}
	}

	return true;
}
