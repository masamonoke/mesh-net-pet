#include "server_handler.h"

#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include "settings.h"
#include "custom_logger.h"
#include "format_server_node.h"
#include "format_client_server.h"
#include "io.h"
#include "server_essentials.h"
#include "connection.h"

static bool send_res_to_client(int32_t client_fd, enum request_result res);

bool handle_ping(const struct node* children, int32_t client_fd, const void* payload) {
	size_t i;
	uint8_t* p;
	struct timeval tv;

	p = (uint8_t*) payload;
	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].label == *p) {
			uint8_t b[256];
			uint32_t buf_len;
			ssize_t received;

			if (children[i].write_fd == -1) {
				custom_log_error("Node killed %d", *p);
				return send_res_to_client(client_fd, REQUEST_ERR);
			}

			if (format_server_node_create_message(REQUEST_PING, NULL, b, &buf_len)) {
				custom_log_error("Failed to create message");
				return false;
			}

			if (io_write_all(children[i].write_fd, (char*) b, buf_len)) {
				custom_log_error("Failed to send request to node");
				return send_res_to_client(client_fd, REQUEST_ERR);
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
				custom_log_error("Failed to get response from node %d", *p);
				return send_res_to_client(client_fd, REQUEST_ERR);
			}

			break;
		}
	}

	return true;
}

static int32_t kill_node(struct node* children, uint8_t label);

bool handle_kill(struct node* children, uint8_t label, int32_t client_fd) { // NOLINT
	enum request_result req_res;

	if (kill_node(children, label)) {
		custom_log_error("Failed to kill node %d", label);
		req_res = REQUEST_ERR;
	} else {
		req_res = REQUEST_OK;
	}

	return send_res_to_client(client_fd, req_res);
}

bool handle_notify(const struct node* children, int32_t client_fd, enum notify_type notify) {
	enum request_result req_res;
	uint8_t b[32];
	uint32_t buf_len;
	bool res;

	req_res = REQUEST_OK;
	res = true;

	switch(notify) {
		case NOTIFY_GOT_MESSAGE:
			if (io_write_all(client_fd, (char*) &req_res, sizeof_enum(req_res))) {
				custom_log_error("Failed to response to notify");
				res = false;
			}
			break;
		case NOTIFY_INVERES_COMPLETED:
			if (format_server_node_create_message(REQUEST_STOP_BROADCAST, NULL, b, &buf_len)) {
				custom_log_error("Failed to create stop broadcast request");
			}

			for (size_t i = 0; i < (size_t) NODE_COUNT; i++) {
				if (children[i].write_fd != -1) {
					if (io_write_all(children[i].write_fd, (char*) b, buf_len)) {
						custom_log_error("Failed to send stop broadcast");
						res = false;
					}
				}
			}
			break;

	}

	return res;
}

static void revivie_node(struct node* node);

bool handle_reset(struct node* children, int32_t client_fd) {
	uint8_t b[32];
	uint32_t buf_len;
	size_t i;
	bool res;

	if (format_server_node_create_message(REQUEST_RESET, NULL, b, &buf_len)) {
		custom_log_error("Failed to create reset nodes request message");
		res = false;
	}

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].write_fd != -1) {
			if (io_write_all(children[i].write_fd, (char*) b, buf_len)) {
				custom_log_error("Failed to send reset request to node %d", children[i].label);
				res = send_res_to_client(client_fd, REQUEST_UNKNOWN);
			}
		} else {
			revivie_node(&children[i]);
		}
	}

	custom_log_debug("Reset nodes");
	res = send_res_to_client(client_fd, REQUEST_OK);

	return res;
}

static bool make_send_to_node(const struct node* children, const void* payload);

bool handle_client_send(struct node* children, const void* payload) {
	uint8_t b[32];
	uint32_t buf_len;

	custom_log_debug("Send command from client");

	if (format_server_node_create_message(REQUEST_RESET_BROADCAST, NULL, b, &buf_len)) {
		custom_log_error("Failed to create stop broadcast request");
	}
	for (size_t i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].write_fd != -1) {
			if (io_write_all(children[i].write_fd, (char*) b, buf_len)) {
				custom_log_error("Failed to send reset broadcast request");
			}
		}
	}

	return make_send_to_node(children, payload);
}

bool handle_revive(struct node* children, uint8_t label, int32_t client_fd) { // NOLINT
	size_t i;
	bool res;

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].label == label) {
			if (children[i].write_fd == -1) {
				revivie_node(&children[i]);
				custom_log_debug("Revived node %d", children[i].label);
				res = send_res_to_client(client_fd, REQUEST_OK);
			} else {
				custom_log_error("Failed to revive node: probably it is not killed");
				res = send_res_to_client(client_fd, REQUEST_ERR);
			}
		}
	}

	return res;
}

void handle_update_child(const void* payload, struct node* children) {
	struct node_update_ret_payload* ret;
	size_t i;

	ret = (struct node_update_ret_payload*) payload;

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].pid == ret->pid) {
			children[i].port = ret->port;
			children[i].label = ret->label;

			children[i].write_fd = connection_socket_to_send((uint16_t) children[i].port);
			if (children[i].write_fd < 0) {
				custom_log_error("Failed to establish connection with node port=%d", children[i].port);
			} else {
				custom_log_debug("Established connection with node: label=%d", children[i].label);
			}
			break;
		}
	}
}

static bool send_res_to_client(int32_t client_fd, enum request_result res) {
	if (io_write_all(client_fd, (char*) &res, sizeof_enum(res))) {
		custom_log_error("Failed to send ping result to client");
		return false;
	}

	return true;
}

static int32_t kill_node(struct node* children, uint8_t label) {
	size_t i;

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].label == label) {
			close(children[i].write_fd);
			children[i].write_fd = -1;
			kill(children[i].pid, SIGTERM);
			custom_log_debug("Killed node %d, pid %d", label, children[i].pid);
			return 0;
		}
	}

	return -1;
}

static bool make_send_to_node(const struct node* children, const void* payload) {
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

static void revivie_node(struct node* node) {
	pid_t pid;

	pid = fork();
	if (pid == 0) {
		run_node(node->label);
	} else {
		node->pid = pid;
	}
	custom_log_debug("Rerun node %d", node->label);
}
