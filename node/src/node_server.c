#include "node_server.h"

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

#include "connection.h"
#include "custom_logger.h"
#include "format.h"
#include "format_client_server.h"
#include "format_node_node.h"
#include "format_server_node.h"
#include "io.h"
#include "settings.h"
#include "control_utils.h"
#include "id.h"
#include "node_essentials.h"
#include "node_handler.h"

static int32_t handle_server(node_server_t* server, int32_t conn_fd, enum request* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data);

static int32_t handle_node(node_server_t* server, enum request* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data);

int32_t node_server_handle_request(node_server_t* server, int32_t conn_fd, uint8_t* buf, ssize_t received_bytes, void* data) {
	enum request request;
	void* payload;
	enum request_sender sender;
	int32_t res;

	sender = format_define_sender(buf);

	switch (sender) {
		case REQUEST_SENDER_SERVER:
			res = handle_server(server, conn_fd, &request, &payload, buf, (size_t) received_bytes, data);
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

static int32_t handle_server(node_server_t* server, int32_t conn_fd, enum request* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data) {
	(void) data;
	int32_t res;

	*payload = NULL;
	if (format_server_node_parse_message(cmd_type, payload, buf, received_bytes)) {
		node_log_error("Failed to parse message");
		return -1;
	}

	res = 0;
	switch (*cmd_type) {
		case REQUEST_PING:
			node_log_debug("Node %d", server->addr);
			res = handle_ping(conn_fd);
			break;
		case REQUEST_SEND:
			node_log_debug("Send request");
			res = handle_server_send(*cmd_type, server->addr, *payload, &server->routing, server->apps);
			break;
		case REQUEST_STOP_BROADCAST:
			handle_stop_broadcast();
			break;
		case REQUEST_RESET_BROADCAST:
			handle_reset_broadcast_status();
			break;
		case REQUEST_RESET:
			if (routing_table_fill_default(&server->routing)) {
				node_log_error("Failed to reset routing table");
			}
			node_essentials_reset_connections();
			node_app_fill_default(server->apps, server->addr);
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

static int32_t handle_node(node_server_t* server, enum request* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data) {
	(void) data;
	int32_t res;

	*payload = NULL;
	if (format_node_node_parse_message(cmd_type, payload, buf, received_bytes)) {
		node_log_error("Failed to parse message: cmd_type %d, node %d", *cmd_type, server->addr);
		return -1;
	}

	res = 0;
	switch (*cmd_type) {
		case REQUEST_SEND:
			res = handle_node_send(server->addr, *payload, &server->routing, server->apps);
			break;
		case REQUEST_ROUTE_DIRECT:
			res = handle_node_route_direct(&server->routing, server->addr, *payload, server->apps);
			break;
		case REQUEST_ROUTE_INVERSE:
			res = handle_node_route_inverse(&server->routing, *payload, server->addr);
			break;
		case REQUEST_UNDEFINED:
			node_log_error("Undefined request: received bytes %d", received_bytes);
			res = -1;
			break;
		default:
			res = -1;
			node_log_error("Unsupported request");
	}

	free(*payload);

	return res;
}
