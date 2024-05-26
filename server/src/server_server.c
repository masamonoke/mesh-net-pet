#include "server_server.h"

#include <stddef.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "connection.h"
#include "custom_logger.h"
#include "format.h"
#include "format_client_server.h"
#include "format_server_node.h"
#include "io.h"

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

static bool handle_ping(const struct node* children, int32_t client_fd, const void* payload);

static bool handle_send(const struct node* children, const void* payload);

static bool handle_client_request(server_t* server_data, void** payload, const uint8_t* buf, size_t received_bytes, void* data) {
	(void) data;
	enum request cmd_type;
	bool res;

	if (format_server_client_parse_message(&cmd_type, payload, buf, received_bytes)) {
		custom_log_error("Failed to parse message");
		return false;
	}

	res = true;
	switch (cmd_type) {
		case REQUEST_SEND:
			{
				uint8_t b[32];
				uint32_t buf_len;

				custom_log_debug("Send command from client");
				if (format_server_node_create_message(REQUEST_RESET_BROADCAST, NULL, b, &buf_len)) {
					custom_log_error("Failed to create stop broadcast request");
				}

				for (size_t i = 0; i < (size_t) NODE_COUNT; i++) {
					if (io_write_all(server_data->children[i].write_fd, (char*) b, buf_len)) {
						custom_log_error("Failed to send reset broadcast request");
					}
				}

				res = handle_send(server_data->children, *payload);
			}
			break;
		case REQUEST_PING:
			custom_log_debug("Ping command from client");
			res = handle_ping(server_data->children, server_data->client_fd, *payload);
			break;
		default:
			res = false;
			break;
	}

	free(*payload);

	return res;
}

static bool handle_send(const struct node* children, const void* payload) {
	char b[256];
	uint32_t buf_len;
	size_t i;

	if (format_server_node_create_message(REQUEST_SEND, payload, (uint8_t*) b, &buf_len)) {
		custom_log_error("Failed to create message");
		return false;
	}

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].label == ((struct send_to_node_ret_payload*) payload)->label_from) {
			if (io_write_all(children[i].write_fd, b, buf_len)) {
				custom_log_error("Failed to send request to node");
				return false;
			}
		}
	}

	return true;
}

static bool handle_ping(const struct node* children, int32_t client_fd, const void* payload) {
	size_t i;
	struct node_ping_ret_payload* p;
	struct timeval tv;

	p = (struct node_ping_ret_payload*) payload;
	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].label == p->label) {
			uint8_t b[256];
			uint32_t buf_len;
			ssize_t received;

			if (format_server_node_create_message(REQUEST_PING, NULL, b, &buf_len)) {
				custom_log_error("Failed to create message");
				return false;
			}

			if (io_write_all(children[i].write_fd, (char*) b, buf_len)) {
				custom_log_error("Failed to send request to node");
				return false;
			}
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			setsockopt(children[i].write_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

			received = recv(children[i].write_fd, b, sizeof(b), 0);

			tv.tv_sec = 0;
			tv.tv_usec = 0;
			setsockopt(children[i].write_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

			if (received > 0) {
				if (io_write_all(client_fd, (char*) b, (size_t) received)) {
					custom_log_error("Failed to send ping result to client");
					return false;
				}
			} else {
				enum request_result res;

				custom_log_error("Failed to get response from node %d", p->label);
				res = REQUEST_ERR;
				if (io_write_all(client_fd, (char*) &res, sizeof_enum(res))) {
					custom_log_error("Failed to send ping result to client");
					return false;
				}
			}

			break;
		}
	}

	return true;
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
				uint8_t b[32];
				uint32_t buf_len;

				req_res = REQUEST_OK;
				if (io_write_all(server_data->client_fd, (char*) &req_res, sizeof_enum(req_res))) {
					custom_log_error("Failed to response to ping");
					res = false;
				}

				if (format_server_node_create_message(REQUEST_STOP_BROADCAST, NULL, b, &buf_len)) {
					custom_log_error("Failed to create stop broadcast request");
				}

				for (size_t i = 0; i < (size_t) NODE_COUNT; i++) {
					if (io_write_all(server_data->children[i].write_fd, (char*) b, buf_len)) {
						custom_log_error("Failed to send stop broadcast");
					}
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

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].pid == ret->pid) {
			children[i].port = ret->port;
			children[i].label = ret->label;
			children[i].initialized = true;

			children[i].write_fd = connection_socket_to_send((uint16_t) children[i].port);
			if (children[i].write_fd < 0) {
				custom_log_error("Failed to establish connection with node port=%d", children[i].port);
			} else {
				custom_log_info("Established connection with node: label=%d", children[i].label);
			}
			break;
		}
	}
}
