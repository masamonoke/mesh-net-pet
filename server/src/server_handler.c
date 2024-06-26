#include "server_handler.h"

#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include "settings.h"
#include "custom_logger.h"
#include "io.h"
#include "server_essentials.h"
#include "connection.h"
#include "crc.h"

__attribute__((warn_unused_result))
static bool send_res_to_client(int32_t client_fd, enum request_result res);

bool handle_ping(const struct node* children, int32_t client_fd, const void* payload) {
	size_t i;
	uint8_t* p;
	struct timeval tv;

	p = (uint8_t*) payload;
	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].addr == *p) {
			uint8_t b[MAX_MSG_LEN];
			msg_len_type buf_len;
			uint8_t received;

			if (children[i].write_fd == -1) {
				custom_log_error("Node killed %d", *p);
				return send_res_to_client(client_fd, REQUEST_ERR);
			}

			format_create(REQUEST_PING, NULL, b, &buf_len, REQUEST_SENDER_SERVER);

			if (!io_write_all(children[i].write_fd, b, buf_len)) {
				custom_log_error("Failed to send request to node");
				return send_res_to_client(client_fd, REQUEST_ERR);
			}
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			setsockopt(children[i].write_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

			received = (uint8_t) recv(children[i].write_fd, b, sizeof(b), 0);

			tv.tv_sec = 0;
			tv.tv_usec = 0;
			setsockopt(children[i].write_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

			if (received > 0) {
				if (!io_write_all(client_fd, b, received)) {
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

__attribute__((warn_unused_result))
static bool kill_node(struct node* children, uint8_t addr);

bool handle_kill(struct node* children, uint8_t addr, int32_t client_fd) { // NOLINT
	enum request_result req_res;

	if (!kill_node(children, addr)) {
		custom_log_error("Failed to kill node %d", addr);
		req_res = REQUEST_ERR;
	} else {
		req_res = REQUEST_OK;
	}

	return send_res_to_client(client_fd, req_res);
}

bool handle_notify(int32_t client_fd, notify_t* notify) {
	enum request_result req_res;
	bool res;

	req_res = REQUEST_OK;
	res = true;

	switch(notify->type) {
		case NOTIFY_GOT_MESSAGE:
			if (client_fd > 0 && !io_write_all(client_fd, (uint8_t*) &req_res, sizeof(req_res))) {
				custom_log_error("Failed to response to notify");
				res = false;
			}
			break;
		case NOTIFY_FAIL:
			req_res = REQUEST_ERR;
			if (client_fd > 0 && !io_write_all(client_fd, (uint8_t*) &req_res, sizeof(req_res))) {
				custom_log_error("Failed to response to notify");
				res = false;
			}
			break;
	}

	return res;
}

static void revivie_node(struct node* node);

bool handle_reset(struct node* children, int32_t client_fd) {
	uint8_t b[sizeof(notify_t) + MSG_BASE_LEN];
	msg_len_type buf_len;
	size_t i;
	bool res;

	format_create(REQUEST_RESET, NULL, b, &buf_len, REQUEST_SENDER_SERVER);

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].write_fd != -1) {
			if (!io_write_all(children[i].write_fd, b, buf_len)) {
				custom_log_error("Failed to send reset request to node %d", children[i].addr);
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

__attribute__((warn_unused_result))
static bool make_send_to_node(const struct node* children, const void* payload);

bool handle_client_send(struct node* children, const void* payload) {
	custom_log_debug("Send command from client");
	return make_send_to_node(children, payload);
}

bool handle_broadcast(struct node* children, const void* payload, enum request cmd) {
	uint8_t b[MAX_MSG_LEN];
	msg_len_type buf_len;
	size_t i;

	format_create(cmd, payload, (uint8_t*) b, &buf_len, REQUEST_SENDER_SERVER);

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].addr == ((node_packet_t*) payload)->sender_addr) {
			if (!io_write_all(children[i].write_fd, b, buf_len)) {
				custom_log_error("Failed to send request to node");
				return false;
			}
		}
	}

	return true;
}

bool handle_revive(struct node* children, uint8_t addr, int32_t client_fd) { // NOLINT
	size_t i;
	bool res;

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].addr == addr) {
			if (children[i].write_fd == -1) {
				revivie_node(&children[i]);
				custom_log_debug("Revived node %d", children[i].addr);
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
	node_update_t* ret;
	size_t i;

	ret = (node_update_t*) payload;

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].pid == ret->pid) {
			children[i].port = ret->port;
			children[i].addr = ret->addr;

			children[i].write_fd = connection_socket_to_send(children[i].port);
			if (children[i].write_fd < 0) {
				custom_log_error("Failed to establish connection with node port=%d", children[i].port);
			} else {
				custom_log_debug("Established connection with node: addr=%d", children[i].addr);
			}
			break;
		}
	}
}

static bool send_res_to_client(int32_t client_fd, enum request_result res) {
	if (!io_write_all(client_fd, (uint8_t*) &res, sizeof(res))) {
		custom_log_error("Failed to send ping result to client");
		return false;
	}

	return true;
}

static bool kill_node(struct node* children, uint8_t addr) {
	size_t i;

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].addr == addr) {
			close(children[i].write_fd);
			children[i].write_fd = -1;
			kill(children[i].pid, SIGTERM);
			custom_log_debug("Killed node %d, pid %d", addr, children[i].pid);
			return true;
		}
	}

	return false;
}

static bool make_send_to_node(const struct node* children, const void* payload) {
	uint8_t b[MAX_MSG_LEN];
	msg_len_type buf_len;
	size_t i;

	((node_packet_t*) payload)->crc = packet_crc(((node_packet_t*) payload));

	format_create(REQUEST_SEND, payload, (uint8_t*) b, &buf_len, REQUEST_SENDER_SERVER);

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		if (children[i].addr == ((node_packet_t*) payload)->sender_addr) {
			if (!io_write_all(children[i].write_fd, b, buf_len)) {
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
		run_node(node->addr);
	} else {
		node->pid = pid;
	}
	custom_log_debug("Rerun node %d", node->addr);
}
