#include "node_server.h"

#include <stdbool.h>               // for false
#include <stdlib.h>                // for size_t, free, NULL
#include <unistd.h>                // for close

#include "connection.h"            // for connection_socket_to_send
#include "custom_logger.h"         // for custom_log_error
#include "format.h"                // for request, request_sender, format_de...
#include "format_client_server.h"  // for send_to_node_ret_payload
#include "format_node_node.h"      // for node_send_payload, format_node_nod...
#include "format_server_node.h"    // for format_server_node_create_message
#include "io.h"                    // for io_write_all
#include "settings.h"              // for node_port, SERVER_PORT

static int32_t handle_server_request(node_server_t* server, int32_t conn_fd, enum request* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data);

static int32_t handle_node(node_server_t* server, enum request* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data);

int32_t node_server_handle_request(node_server_t* server, int32_t conn_fd, uint8_t* buf, ssize_t received_bytes, void* data) {
	enum request request;
	void* payload;
	enum request_sender sender;
	int32_t res;

	sender = format_define_sender(buf);

	switch (sender) {
		case REQUEST_SENDER_SERVER:
			res = handle_server_request(server, conn_fd, &request, &payload, buf, (size_t) received_bytes, data);
			break;
		case REQUEST_SENDER_NODE:
			res = handle_node(server, &request, &payload,  buf, (size_t) received_bytes, data);
			break;
		default:
			res = -1;
			custom_log_error("Unsupported request sender");
	}

	return res;
}

static int32_t got_message_notify(void);

static int32_t handle_ping(int32_t conn_fd);

static int32_t handle_server_send(enum request cmd_type, int32_t label, const void* payload, const routing_table_t* routing);

static int32_t handle_server_request(node_server_t* server, int32_t conn_fd, enum request* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data) {
	(void) data;
	int32_t res;

	*payload = NULL;
	if (format_server_node_parse_message(cmd_type, payload, buf, received_bytes)) {
		node_log_error("Failed to parse message");
		return false;
	}

	res = 0;
	switch (*cmd_type) {
		case REQUEST_PING:
			res = handle_ping(conn_fd);
			break;
		case REQUEST_SEND:
			node_log_debug("Send request");
			res = handle_server_send(*cmd_type, server->label, *payload, &server->routing);
			break;
		case REQUEST_UNDEFINED:
			node_log_error("Undefined server-node request type");
			res = -1;
			break;
		default:
			node_log_error("Unsupported request");
			res = -1;
	}

	free(*payload);

	return res;
}

static int32_t handle_node_send(int32_t label, const void* payload, const routing_table_t* routing, const uint8_t* buf, size_t received_bytes) {
	int32_t label_to;
	int32_t res;

	res = 0;
	label_to = ((struct node_send_payload*) payload)->label_to;
	if (label_to == label) {
		if (got_message_notify()) {
			node_log_error("Failed to notify");
			res = -1;
			return res;
		}
	} else {
		struct node_send_payload* ret_payload;
		ssize_t i;
		int32_t node_conn;

		ret_payload = (struct node_send_payload*) payload;
		node_log_debug("Finding route to %d", ret_payload->label_to);
		i = routing_table_get_dest_idx(routing, ret_payload->label_to);
		if (i < 0) {
			node_log_error("Failed to find route");
			res = -1;
			return res;
		}

		node_conn = connection_socket_to_send(node_port(routing->nodes[i].label));

		if (node_conn < 0) {
			node_log_error("Failed to create connection with node %d", ret_payload->label_to);
		} else {
			if (io_write_all(node_conn, (char*) buf, received_bytes)) {
				node_log_error("Failed to send request");
				res = -1;
			}

			close(node_conn);
		}
	}

	return res;
}

static int32_t handle_node(node_server_t* server, enum request* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data) {
	(void) data;
	int32_t res;

	*payload = NULL;
	if (format_node_node_parse_message(cmd_type, payload, buf, received_bytes)) {
		node_log_error("Failed to parse message");
		return false;
	}

	res = 0;
	switch (*cmd_type) {
		case REQUEST_SEND:
			res = handle_node_send(server->label, *payload, &server->routing, buf, received_bytes);
			break;
		case REQUEST_UNDEFINED:
			node_log_error("Undefined request");
			res = -1;
			break;
		default:
			res = -1;
			node_log_error("Unsupported request");
	}

	free(*payload);

	return res;
}

static int32_t got_message_notify(void) {
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

	server_fd = connection_socket_to_send(SERVER_PORT);
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

static int32_t handle_server_send(enum request cmd_type, int32_t label, const void* payload, const routing_table_t* routing) { // NOLINT
	struct send_to_node_ret_payload* ret_payload;
	uint8_t b[256];
	uint32_t buf_len;
	int32_t node_conn;
	ssize_t i;
	int32_t res;

	res = 0;
	ret_payload = (struct send_to_node_ret_payload*) payload;
	if (ret_payload->label_to == label) {
		node_log_warn("Message for node itself");
		if (got_message_notify()) {
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
	i = routing_table_get_dest_idx(routing, ret_payload->label_to);
	if (i < 0) {
		node_log_error("Failed to find route");
		res = -1;
		return res;
	}

	node_conn = connection_socket_to_send(node_port(routing->nodes[i].label));

	if (node_conn < 0) {
		node_log_error("Failed to create connection with node %d", ret_payload->label_to);
		res = -1;
	} else {
		if (io_write_all(node_conn, (char*) b, buf_len)) {
			node_log_error("Failed to send request");
			res = -1;
		}

		close(node_conn);

		node_log_debug("Intermediate node %d traversing to %d", label, ret_payload->label_to);
	}

	return 0;
}

static int32_t handle_ping(int32_t conn_fd) {
	enum request_result req_res;

	req_res = REQUEST_OK;
	if (io_write_all(conn_fd, (char*) &req_res, sizeof(req_res))) {
		node_log_error("Failed to response to ping");
		return -1;
	}

	return 0;
}
