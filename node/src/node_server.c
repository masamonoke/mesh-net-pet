#include "node_server.h"

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

#include "custom_logger.h"
#include "format.h"
#include "format_node_node.h"
#include "format_server_node.h"
#include "node_essentials.h"
#include "node_handler.h"

__attribute__((warn_unused_result))
static bool handle_server(node_server_t* server, int32_t conn_fd, enum request* cmd_type, void** payload, uint8_t* buf, void* data);

__attribute__((warn_unused_result))
static bool handle_node(node_server_t* server, enum request* cmd_type, void** payload, uint8_t* buf, size_t received_bytes, void* data);

bool node_server_handle_request(node_server_t* server, int32_t conn_fd, uint8_t* buf, ssize_t received_bytes, void* data) {
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
	format_server_node_parse_message(cmd_type, payload, buf);

	res = true;
	switch (*cmd_type) {
		case REQUEST_PING:
			node_log_info("Ping node %d", server->addr);
			res = handle_ping(conn_fd);
			break;
		case REQUEST_SEND:
			res = handle_server_send(*cmd_type, server->addr, *payload, &server->routing, server->apps);
			break;
		case REQUEST_STOP_BROADCAST:
			handle_stop_broadcast();
			break;
		case REQUEST_RESET_BROADCAST:
			handle_reset_broadcast_status();
			break;
		case REQUEST_RESET:
			routing_table_fill_default(&server->routing);
			node_essentials_reset_connections();
			node_app_fill_default(server->apps, server->addr);
			break;
		case REQUEST_BROADCAST:
		case REQUEST_UNICAST:
			{
				uint8_t b[MAX_MSG_LEN];
				uint8_t buf_len;
				send_t send_payload = {
					.app_payload = ((broadcast_t*) *payload)->app_payload,
					.addr_from = server->addr,
					.addr_to = server->addr
				};

				format_server_node_create_message(REQUEST_SEND, &send_payload, b, &buf_len);
				res = handle_server_send(*cmd_type, server->addr, &send_payload, &server->routing, server->apps);
				handle_broadcast(*payload);
			}
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
	format_node_node_parse_message(cmd_type, payload, buf);

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
