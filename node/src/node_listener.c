#include "node_listener.h"

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

#include "custom_logger.h"
#include "format.h"
#include "node_essentials.h"
#include "node_handler.h"

__attribute__((warn_unused_result))
static bool handle_server(node_server_t* server, int32_t conn_fd, enum request* cmd_type, void** payload, uint8_t* buf, void* data);

__attribute__((warn_unused_result))
static bool handle_node(node_server_t* server, enum request* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data);

bool node_listener_handle_request(node_server_t* server, int32_t conn_fd, uint8_t* buf, ssize_t received_bytes, void* data) {
	enum request request;
	void* payload;
	enum request_sender sender;
	bool res;

	sender = format_define_sender(buf);

	switch (sender) {
		case REQUEST_SENDER_SERVER:
			res = handle_server(server, conn_fd, &request, &payload, buf, data);
			break;
		case REQUEST_SENDER_NODE:
			res = handle_node(server, &request, &payload,  buf, (size_t) received_bytes, data);
			break;
		default:
			res = false;
			custom_log_error("Unsupported request sender");
	}

	return res;
}

static bool handle_server(node_server_t* server, int32_t conn_fd, enum request* cmd_type, void** payload, uint8_t* buf, void* data) {
	(void) data;
	bool res;

	*payload = NULL;
	format_parse(cmd_type, payload, buf);

	res = true;
	switch (*cmd_type) {
		case REQUEST_PING:
			node_log_info("Ping node %d", server->addr);
			res = handle_ping(conn_fd);
			break;
		case REQUEST_SEND:
			res = handle_server_send(*cmd_type, server->addr, *payload, &server->routing, server->apps);
			break;
		case REQUEST_RESET:
			routing_table_fill_default(&server->routing);
			node_essentials_reset_connections();
			node_app_fill_default(server->apps, server->addr);
			break;
		case REQUEST_UNICAST:
			handle_server_unicast(*payload, server->addr);
			break;
		case REQUEST_BROADCAST:
			handle_broadcast(*payload);
			break;
		case REQUEST_UNDEFINED:
			node_log_error("Undefined server-node request type");
			res = false;
			break;
		default:
			node_log_error("Unsupported request");
			res = false;
	}

	free(*payload);

	return res;
}

static bool handle_node(node_server_t* server, enum request* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data) {
	(void) data;
	bool res;

	*payload = NULL;
	format_parse(cmd_type, payload, buf);

	res = true;
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
		case REQUEST_UNICAST_CONTEST:
			handle_unicast_contest(*payload, server->addr);
			break;
		case REQUEST_UNICAST_FIRST:
			handle_unicast_first(*payload, server->addr);
			break;
		case REQUEST_UNDEFINED:
			node_log_error("Undefined request: received bytes %d", received_bytes);
			res = false;
			break;
		default:
			res = false;
			node_log_error("Unsupported request");
	}

	free(*payload);

	return res;
}
