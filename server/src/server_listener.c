#include "server_listener.h"

#include <stddef.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "custom_logger.h"
#include "format.h"
#include "format.h"
#include "server_handler.h"

static uint16_t app_msg_id = 0;

static void init_clients(server_t* server_data);

static void set_client(server_t* server_data, uint16_t id, int32_t fd);

__attribute__((nonnull(1), warn_unused_result))
static int32_t get_client_fd_by_id(server_t* server_data, uint16_t id);

__attribute__((warn_unused_result))
static bool handle_client_request(server_t* server_data, void** payload, const uint8_t* buf, void* data);

__attribute__((warn_unused_result))
static bool handle_node_request(server_t* server_data, void** payload, const uint8_t* buf, void* data);

bool server_listener_handle(server_t* server, const uint8_t* buf, int32_t conn_fd, void* data) {
	bool processed;
	enum request_sender sender;
	void* payload;

	sender = format_define_sender(buf);

	switch(sender) {
		case REQUEST_SENDER_CLIENT:
			server->client_fd = conn_fd;
			processed = handle_client_request(server, &payload, buf, data);
			break;
		case REQUEST_SENDER_NODE:
			processed = handle_node_request(server, &payload, buf, data);
			break;
		default:
			processed = false;
			custom_log_error("Unknown request sender");
			break;
	}

	return processed;
}

static bool handle_client_request(server_t* server_data, void** payload, const uint8_t* buf, void* data) {
	(void) data;
	enum request cmd_type;
	bool res;

	*payload = NULL;
	format_parse(&cmd_type, payload, buf);

	res = true;
	switch (cmd_type) {
		case REQUEST_SEND:
			((node_packet_t*) *payload)->app_payload.id = app_msg_id++;
			set_client(server_data, ((node_packet_t*) *payload)->app_payload.id, server_data->client_fd);
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
		case REQUEST_BROADCAST:
		case REQUEST_UNICAST:
			((node_packet_t*) *payload)->app_payload.id = app_msg_id++;
			set_client(server_data, ((node_packet_t*) *payload)->app_payload.id, server_data->client_fd);
			res = handle_broadcast(server_data->children, *payload, cmd_type);
			break;
		default:
			custom_log_error("Unknown client request: %d", cmd_type);
			res = false;
			break;
	}

	free(*payload);

	return res;
}

static bool handle_node_request(server_t* server_data, void** payload, const uint8_t* buf, void* data) {
	enum request cmd_type;
	bool res;
	int32_t client_fd;

	*payload = NULL;
	format_parse(&cmd_type, payload, buf);

	res = true;
	switch (cmd_type) {
		case REQUEST_UPDATE:
			handle_update_child(*payload, data);
			break;
		case REQUEST_NOTIFY:
			client_fd = get_client_fd_by_id(server_data, ((notify_t*) *payload)->app_msg_id);
			res = handle_notify(client_fd,  *payload);
			set_client(server_data, ((notify_t*) *payload)->app_msg_id, -1);
			break;
		default:
			custom_log_error("Unsupported request");
			res = false;
			break;
	}

	free(*payload);

	return res;
}

static void init_clients(server_t* server_data) {
	static bool init = false;

	if (!init) {
		uint8_t i;

		for (i = 0; i < MAX_CLIENT; i++) {
			server_data->clients[i].fd = -1;
		}
		server_data->client_num = 0;
		init = true;
	}
}

static void set_client(server_t* server_data, uint16_t id, int32_t fd) {
	init_clients(server_data);

	if (server_data->client_num == MAX_CLIENT) {
		server_data->client_num = 0;
	}

	server_data->clients[server_data->client_num].app_id = id;
	server_data->clients[server_data->client_num].fd = fd;
	server_data->client_num++;
}

static int32_t get_client_fd_by_id(server_t* server_data, uint16_t id) {
	uint8_t i;
	int32_t fd;

	init_clients(server_data);

	for (i = 0; i < server_data->client_num; i++) {
		if (server_data->clients[i].app_id == id) {
			fd = server_data->clients[i].fd;
			server_data->clients[i].fd = -1;
			return fd;
		}
	}

	return -1;
}
