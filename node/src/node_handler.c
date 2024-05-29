#include "node_handler.h"

#include <memory.h>

#include "node_essentials.h"
#include "io.h"
#include "format_client_server.h"
#include "control_utils.h"
#include "node_app.h"

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

int32_t handle_server_send(enum request cmd_type, uint8_t label, const void* payload, const routing_table_t* routing, app_t apps[APPS_COUNT]) { // NOLINT
	struct send_to_node_ret_payload* ret_payload;
	uint8_t b[256];
	uint32_t buf_len;
	int32_t node_conn;
	uint8_t next_label;
	int32_t res;

	res = 0;
	ret_payload = (struct send_to_node_ret_payload*) payload;

	if (ret_payload->label_to == label) {
		node_log_warn("Message for node itself");

		if (node_app_handle_request(apps, &ret_payload->app_payload, 0)) {
			node_essentials_notify_server(NOTIFY_GOT_MESSAGE);
		}

		return res;
	}

	node_app_setup_delivery(apps, &ret_payload->app_payload, ret_payload->label_to);

	if (format_node_node_create_message(cmd_type, ret_payload, b, &buf_len)) {
		node_log_error("Failed to create message");
		res = -1;
		return res;
	}

	node_log_debug("Finding route to %d", ret_payload->label_to);
	next_label = routing_next_label(routing, ret_payload->label_to);
	if (next_label == UINT8_MAX) {
		node_log_error("Failed to find route");

		struct node_route_direct_payload route_payload = {
			.local_sender_label = label,
			.sender_label = ret_payload->label_from,
			.receiver_label = ret_payload->label_to,
			.metric = 0,
			.time_to_live = TTL,
			.app_payload = ret_payload->app_payload
			// id is not used yet
		};

		node_essentials_broadcast(label, UINT8_MAX, &route_payload, stop_broadcast);

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

static bool send_delivery(const routing_table_t* routing, uint8_t old_from, uint8_t old_to, struct app_payload* old_app_payload, app_t apps[APPS_COUNT]);

static bool send_key_exchange(const routing_table_t* routing, struct app_payload* app_payload, uint8_t receiver_label, uint8_t sender_label);

static bool node_handle_app_request(const routing_table_t* routing, app_t apps[APPS_COUNT], struct send_to_node_ret_payload* send_payload);

static bool send_next(const routing_table_t* routing, struct send_to_node_ret_payload* ret_payload);

int32_t handle_node_send(uint8_t label, const void* payload, const routing_table_t* routing, app_t apps[APPS_COUNT]) {
	uint8_t label_to;
	int32_t res;

	node_log_warn("Send node %d", label);

	res = 0;
	label_to = ((struct send_to_node_ret_payload*) payload)->label_to;
	if (label_to == label) {
		node_handle_app_request(routing, apps, (struct send_to_node_ret_payload*) payload);
	} else {
		send_next(routing, (struct send_to_node_ret_payload*) payload);
	}

	return res;
}

bool route_direct_handle_delivered(routing_table_t* routing, struct node_route_direct_payload* route_payload, uint8_t server_label, app_t apps[APPS_COUNT]);

int32_t handle_node_route_direct(routing_table_t* routing, uint8_t server_label, void* payload, app_t apps[APPS_COUNT]) {
	struct node_route_direct_payload* route_payload;
	uint8_t prev_label;

	route_payload = (struct node_route_direct_payload*) payload;

	if (was_message) {
		return 0;
	} else {
		was_message = true;
	}

	if (routing_next_label(routing, route_payload->sender_label) == UINT8_MAX) {
		routing_set_label(routing, route_payload->sender_label, route_payload->local_sender_label, route_payload->metric);
	} else {
		if (routing_get(routing, route_payload->sender_label).metric > route_payload->metric) {
			routing_set_label(routing, route_payload->sender_label, route_payload->local_sender_label, route_payload->metric);
		}
	}

	if (route_payload->receiver_label == server_label) {
		route_direct_handle_delivered(routing, route_payload, server_label, apps);
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

int32_t handle_node_route_inverse(routing_table_t* routing, void* payload, uint8_t server_label) {
	struct node_route_inverse_payload* route_payload;
	int32_t conn_fd;
	uint8_t next_label;
	uint8_t b[256];
	uint32_t buf_len;

	route_payload = (struct node_route_inverse_payload*) payload;

	node_log_warn("Inverse node %d", server_label);

	if (routing_next_label(routing, route_payload->receiver_label) == UINT8_MAX) {
		routing_set_label(routing, route_payload->receiver_label, route_payload->local_sender_label, route_payload->metric);
	}

	if (route_payload->sender_label == server_label) {
		node_log_info("Route inverse request came back");
		return 0;
	}

	next_label = routing_next_label(routing, route_payload->sender_label);
	if (next_label == UINT8_MAX) {
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
		node_essentials_notify_server(NOTIFY_INVERES_COMPLETED);
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

static bool send_delivery(const routing_table_t* routing, uint8_t old_from, uint8_t old_to, struct app_payload* old_app_payload, app_t apps[APPS_COUNT]) {
	uint8_t b[256];
	uint32_t buf_len;
	uint8_t next_label;
	int32_t node_conn;

	struct send_to_node_ret_payload new_send = {
		.label_to = old_from,
		.label_from = old_to,
		.app_payload.label_to = old_app_payload->label_from,
		.app_payload.label_from = old_app_payload->label_to,
		.app_payload.req_type = APP_REQUEST_DELIVERY,
		.app_payload.key = old_app_payload->key
	};

	node_app_setup_delivery(apps, &new_send.app_payload, new_send.label_to);

	if (format_node_node_create_message(REQUEST_SEND, &new_send, b, &buf_len)) {
		node_log_error("Faield to create send request");
		return false;
	}

	next_label = routing_next_label(routing, new_send.label_to);
	if (next_label == UINT8_MAX) {
		node_log_error("Failed to find path in table");
		return false;
	}

	node_conn = node_essentials_get_conn(node_port(next_label));

	if (io_write_all(node_conn, (char*) b, buf_len)) {
		node_log_error("Failed to send request");
		return false;
	}

	return true;
}

static bool send_key_exchange(const routing_table_t* routing, struct app_payload* app_payload, uint8_t receiver_label, uint8_t sender_label) {
	uint8_t tmp;
	uint8_t b[256];
	uint32_t buf_len;
	uint8_t next_label;
	int32_t node_conn;

	node_log_warn("Exchanged keys between %d:%d (current) and %d:%d",
		receiver_label, app_payload->label_to, sender_label, app_payload->label_from);

	tmp = app_payload->label_from;
	app_payload->label_from = app_payload->label_to;
	app_payload->label_to = tmp;

	struct send_to_node_ret_payload send_payload = {
		.label_to = sender_label,
		.label_from = receiver_label,
		.app_payload = *app_payload
	};
	if (format_node_node_create_message(REQUEST_SEND, &send_payload, b, &buf_len)) {
		node_log_error("Failed to create send request");
		return false;
	}

	next_label = routing_next_label(routing, send_payload.label_to);
	if (next_label == UINT8_MAX) {
		node_log_error("Failed to find path to %d in table", send_payload.label_to);

		struct node_route_direct_payload route_payload = {
			.local_sender_label = receiver_label,
			.sender_label = receiver_label,
			.receiver_label = sender_label,
			.metric = 0,
			.time_to_live = TTL,
			.app_payload = *app_payload
			// id is not used yet
		};

		node_log_warn("Sending broadcast");
		node_essentials_broadcast(receiver_label, UINT8_MAX, &route_payload, stop_broadcast);

		return true;
	}

	node_conn = node_essentials_get_conn(node_port(next_label));

	if (io_write_all(node_conn, (char*) b, buf_len)) {
		node_log_error("Failed to send request");
		return false;
	}

	return true;
}

static bool node_handle_app_request(const routing_table_t* routing, app_t apps[APPS_COUNT], struct send_to_node_ret_payload* send_payload) {
	enum app_request app_req;
	bool res;

	app_req = send_payload->app_payload.req_type;
	res = true;

	switch (app_req) {
		case APP_REQUEST_EXCHANGED_KEY:
			node_log_debug("Get exchange key notify. Sending message");
			if (node_app_save_key(apps, &send_payload->app_payload, send_payload->label_from)) {
				send_delivery(routing, send_payload->label_from, send_payload->label_to, &send_payload->app_payload, apps);
			} else {
				node_log_error("Failed to save key");
				res = false;
			}
			break;
		case APP_REQUEST_DELIVERY:
				if (node_app_handle_request(apps, &send_payload->app_payload, send_payload->label_from)) {
					node_essentials_notify_server(NOTIFY_GOT_MESSAGE);
				} else {
					res = false;
				}
			break;
		case APP_REQUEST_KEY_EXCHANGE:
			if (node_app_handle_request(apps, &send_payload->app_payload, send_payload->label_from)) {
				if (send_payload->app_payload.req_type == APP_REQUEST_EXCHANGED_KEY) {
					if (!send_key_exchange(routing, &send_payload->app_payload, send_payload->label_to, send_payload->label_from)) {
						node_log_error("Failed to send key exchange response");
						res = false;
					}
				}
			} else {
				res = false;
			}
			break;
	}

	return res;
}

static bool send_next(const routing_table_t* routing, struct send_to_node_ret_payload* ret_payload) {
	int32_t node_conn;
	uint8_t next_label;

	next_label = routing_next_label(routing, ret_payload->label_to);
	if (next_label == UINT8_MAX) {
		node_log_error("Failed to find path in table");
		// TODO: this may happen if node died after path was found
		// start broadcast from here
		return false;
	}

	node_conn = node_essentials_get_conn(node_port(next_label));

	if (node_conn < 0) {
		node_log_error("Failed to create connection with node %d", ret_payload->label_to);
		// TODO: this may happen if next label from routing table is died
		// delete old label from routing and start broadcast from here
		return false;
	} else {
		uint8_t b[256];
		uint32_t buf_len;

		if (format_node_node_create_message(REQUEST_SEND, ret_payload, b, &buf_len)) {
			node_log_error("Failed to create node-send message");
			return false;
		}

		if (io_write_all(node_conn, (char*) b, buf_len)) {
			node_log_error("Failed to send request");
			return false;
		}
	}

	return true;
}

bool route_direct_handle_delivered(routing_table_t* routing, struct node_route_direct_payload* route_payload, uint8_t server_label, app_t apps[APPS_COUNT]) {
	uint8_t b[256];
	uint32_t buf_len;
	uint8_t next_label_to_back;
	int32_t conn_fd;

	node_log_info("Reached the recevier label %d", route_payload->receiver_label);

	if (!stop_inverse) {
		// replace with finding id function
		stop_inverse = true;
	} else {
		// somebody alreaady reached this label with this message id and coming back
		return true;
	}

	route_payload->metric = 0;
	route_payload->time_to_live = TTL;
	route_payload->local_sender_label = server_label;

	if (format_node_node_create_message(REQUEST_ROUTE_INVERSE, route_payload, b, &buf_len)) {
		node_log_error("Failed to create route inverse message");
		return false;
	}

	next_label_to_back = routing_next_label(routing, route_payload->sender_label);
	if (next_label_to_back == UINT8_MAX) {
		node_log_error("Failed to get next label to %d", route_payload->sender_label);
		return false;
	}

	conn_fd = node_essentials_get_conn(node_port(next_label_to_back));

	if (conn_fd < 0) {
		node_log_error("Failed to connect to %d node", next_label_to_back);
		routing_del(routing, route_payload->sender_label);
		return false;
	}

	if (io_write_all(conn_fd, (char*) b, buf_len)) {
		node_log_error("Failed to send route inverse request");
		return false;
	}

	switch (route_payload->app_payload.req_type) {
		case APP_REQUEST_KEY_EXCHANGE:
			if (node_app_handle_request(apps, &route_payload->app_payload, route_payload->sender_label)) {
				if (route_payload->app_payload.req_type == APP_REQUEST_EXCHANGED_KEY) {
					if (!send_key_exchange(routing, &route_payload->app_payload, route_payload->receiver_label, route_payload->sender_label)) {
						node_log_error("Failed to send key exchange response");
					}
				}
			}
			break;
		case APP_REQUEST_EXCHANGED_KEY:
				node_log_debug("Get exchange key notify. Sending message");
				send_delivery(routing, route_payload->sender_label, route_payload->receiver_label, &route_payload->app_payload, apps);
			break;
		default:
			node_log_error("Unexpected app request: %d", route_payload->app_payload.req_type);
			break;
	}

	return true;
}
