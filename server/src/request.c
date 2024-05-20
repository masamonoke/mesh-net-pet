#include "request.h"

#include <stddef.h>                // for size_t, NULL
#include <stdlib.h>                // for free
#include <sys/socket.h>            // for setsockopt, SOL_SOCKET, SO_RCVTIMEO
#include <sys/time.h>              // for timeval

#include "connection.h"            // for connection_socket_to_send
#include "custom_logger.h"         // for custom_log_error, custom_log_debug
#include "format.h"                // for request, request_result, request_s...
#include "format_client_server.h"  // for format_server_client_parse_message
#include "format_server_node.h"    // for format_server_node_create_message
#include "io.h"                    // for io_write_all

static bool handle_client_request(server_t* server_data, void** payload, const uint8_t* buf, size_t received_bytes, void* data);

static bool handle_node_request(const server_t* server_data, void** payload, const uint8_t* buf, size_t received_bytes, void* data);

bool request_handle(server_t* server, const uint8_t* buf, ssize_t received_bytes, int32_t conn_fd, void* data) { // NOLINT
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

static void handle_ping(const struct node* children, int32_t client_fd, const void* payload);

static bool handle_send(const struct node* children, const void* payload);

static bool handle_client_request(server_t* server_data, void** payload, const uint8_t* buf, size_t received_bytes, void* data) {
	(void) data;
	enum request cmd_type;

	if (format_server_client_parse_message(&cmd_type, payload, buf, received_bytes)) {
		custom_log_error("Failed to parse message");
		return false;
	}

	switch (cmd_type) {
		case REQUEST_SEND:
			custom_log_debug("Send command from client");
			handle_send(server_data->children, *payload);
			break;
		case REQUEST_PING:
			custom_log_debug("Ping command from client");
			handle_ping(server_data->children, server_data->client_fd, *payload);
			break;
		default:
			return false;
			break;
	}

	free(*payload);

	return true;
}

static bool handle_send(const struct node* children, const void* payload) {
	char b[256];
	uint32_t buf_len;
	size_t i;

	if (format_server_node_create_message(REQUEST_SEND, payload, (uint8_t*) b, &buf_len)) {
		custom_log_error("Failed to create message");
		return false;
	}

	for (i = 0; i < NODE_COUNT; i++) {
		if (children[i].label == ((struct send_to_node_ret_payload*) payload)->label_from) {
			if (io_write_all(children[i].write_fd, b, buf_len)) {
				custom_log_error("Failed to send request to node");
				return false;
			}
		}
	}

	return true;
}

static void handle_ping(const struct node* children, int32_t client_fd, const void* payload) {
	size_t i;
	struct node_ping_ret_payload* p;
	struct timeval tv;

	p = (struct node_ping_ret_payload*) payload;
	for (i = 0; i < NODE_COUNT; i++) {
		if (children[i].label == p->label) {
			uint8_t b[256];
			uint32_t buf_len;
			ssize_t received;

			if (format_server_node_create_message(REQUEST_PING, NULL, b, &buf_len)) {
				custom_log_error("Failed to create message");
				return;
			}

			if (io_write_all(children[i].write_fd, (char*) b, buf_len)) {
				custom_log_error("Failed to send request to node");
			}
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			setsockopt(children[i].write_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

			received = recv(children[i].write_fd, b, sizeof(b), 0);

			tv.tv_sec = 0;
			tv.tv_usec = 0;
			setsockopt(children[i].write_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

			if (received > 0) {
				if (io_write_all(client_fd, (char*) b, (size_t) received)) {
					custom_log_error("Failed to send ping result to client");
				}
			} else {
				enum request_result res;

				custom_log_error("Failed to get response from node %d", p->label);
				res = REQUEST_ERR;
				if (io_write_all(client_fd, (char*) &res, sizeof(res))) {
					custom_log_error("Failed to send ping result to client");
				}
			}

			break;
		}
	}
}

static void handle_update_child(const void* payload, struct node* children);

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
			{
				enum request_result req_res;

				req_res = REQUEST_OK;
				if (io_write_all(server_data->client_fd, (char*) &req_res, sizeof(req_res))) {
					custom_log_error("Failed to response to ping");
					res = false;
				}
			}
			break;
		default:
			custom_log_error("Unsupported request");
			res = false;
			break;
	}

	free(*payload);

	return res;
}

static void handle_update_child(const void* payload, struct node* children) {
	struct node_update_ret_payload* ret;
	size_t i;

	ret = (struct node_update_ret_payload*) payload;

	for (i = 0; i < NODE_COUNT; i++) {
		if (children[i].pid == ret->pid) {
			children[i].port = ret->port;
			children[i].label = ret->label;
			children[i].alias = ret->alias;
			children[i].initialized = true;

			children[i].write_fd = connection_socket_to_send((uint16_t) children[i].port);
			if (children[i].write_fd < 0) {
				custom_log_error("Failed to establish connection with node port=%d, alias=%s", children[i].port, children[i].alias);
			} else {
				custom_log_info("Established connection with node: label=%d, alias=%s", children[i].label, children[i].alias);
			}
			break;
		}
	}
}
