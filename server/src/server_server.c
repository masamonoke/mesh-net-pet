#include "server_server.h"

#include <stddef.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include "connection.h"
#include "custom_logger.h"
#include "format.h"
#include "format_client_server.h"
#include "format_server_node.h"
#include "io.h"
#include "control_utils.h"
#include "server_essentials.h"
#include "server_handler.h"

static bool handle_client_request(server_t* server_data, void** payload, const uint8_t* buf, size_t received_bytes, void* data);

static bool handle_node_request(const server_t* server_data, void** payload, const uint8_t* buf, size_t received_bytes, void* data);

bool server_server_handle(server_t* server, const uint8_t* buf, ssize_t received_bytes, int32_t conn_fd, void* data) { // NOLINT
	bool processed;
	enum request_sender sender;
	void* payload;

	sender = format_define_sender(buf);

	switch(sender) {
		case REQUEST_SENDER_CLIENT:
			server->client_fd = conn_fd;
			processed = handle_client_request(server, &payload, buf, (size_t) received_bytes, data);
			break;
		case REQUEST_SENDER_NODE:
			processed = handle_node_request(server, &payload, buf, (size_t) received_bytes, data);
			break;
		default:
			processed = false;
			custom_log_error("Unknown request sender");
			break;
	}

	return processed;
}

static bool handle_client_request(server_t* server_data, void** payload, const uint8_t* buf, size_t received_bytes, void* data) {
	(void) data;
	enum request cmd_type;
	bool res;

	*payload = NULL;
	if (format_server_client_parse_message(&cmd_type, payload, buf, received_bytes)) {
		custom_log_error("Failed to parse message");
		return false;
	}

	res = true;
	switch (cmd_type) {
		case REQUEST_SEND:
			res = handle_client_send(server_data->children, *payload);
			break;
		case REQUEST_PING:
			custom_log_debug("Ping command from client");
			res = handle_ping(server_data->children, server_data->client_fd, *payload);
			break;
		case REQUEST_KILL_NODE:
			res = handle_kill(server_data->children, *((uint8_t*) *payload), server_data->client_fd);
			break;
		case REQUEST_RESET:
			res = handle_reset(server_data->children, server_data->client_fd);
			break;
		case REQUEST_REVIVE_NODE:
			res = handle_revive(server_data->children, *((uint8_t*) *payload), server_data->client_fd);
			break;
		default:
			res = false;
			break;
	}

	free(*payload);

	return res;
}

static bool handle_node_request(const server_t* server_data, void** payload, const uint8_t* buf, size_t received_bytes, void* data) {
	enum request cmd_type;
	bool res;

	*payload = NULL;
	if (format_server_node_parse_message(&cmd_type, payload, buf, received_bytes)) {
		custom_log_error("Failed to parse message");
		return false;
	}

	res = true;
	switch (cmd_type) {
		case REQUEST_UPDATE:
			handle_update_child(*payload, data);
			break;
		case REQUEST_NOTIFY:
			res = handle_notify(server_data->children, server_data->client_fd, ((struct node_notify_payload*) *payload)->notify_type);
			break;
		default:
			custom_log_error("Unsupported request");
			res = false;
			break;
	}

	free(*payload);

	return res;
}
