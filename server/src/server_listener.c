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
#include "crc.h"
#include "format_app.h"

#define MAX_CLIENT 4

static uint16_t app_msg_id = 0;

typedef struct client {
	int32_t fd;
	uint16_t app_id;
} client_t;

client_t clients[MAX_CLIENT];
uint8_t client_num;

static void init_clients(void);

static void set_client(uint16_t id, int32_t fd);

__attribute__((warn_unused_result))
static int32_t get_client_fd_by_id(uint16_t id);

__attribute__((warn_unused_result))
static bool handle_client_request(server_t* server_data, void** payload, const uint8_t* buf, void* data);

__attribute__((warn_unused_result))
static bool handle_node_request(void** payload, const uint8_t* buf, void* data);

void server_listener_init(void) {
	init_clients();
}

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
			processed = handle_node_request(&payload, buf, data);
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
			{
				node_packet_t* packet;
				struct app_payload* app_ptr;

				packet = (node_packet_t*) *payload;
				packet->app_payload.id = app_msg_id++;
				app_ptr = &packet->app_payload;
				packet->app_payload.crc = app_crc(app_ptr);
				set_client(((node_packet_t*) *payload)->app_payload.id, server_data->client_fd);
				res = handle_client_send(server_data->children, *payload);
			}
			break;
		case REQUEST_PING:
			custom_log_debug("Ping command from client");
			res = handle_ping(server_data->children, server_data->client_fd, *payload);
			break;
		case REQUEST_KILL_NODE:
			res = handle_kill(server_data->children, *((uint8_t*) *payload), server_data->client_fd);
			break;
		case REQUEST_RESET:
			app_msg_id = 0;
			res = handle_reset(server_data->children, server_data->client_fd);
			break;
		case REQUEST_REVIVE_NODE:
			res = handle_revive(server_data->children, *((uint8_t*) *payload), server_data->client_fd);
			break;
		case REQUEST_BROADCAST:
		case REQUEST_UNICAST:
			{
				node_packet_t* packet;
				struct app_payload* app_ptr;

				packet = (node_packet_t*) *payload;
				packet->app_payload.id = app_msg_id++;
				app_ptr = &packet->app_payload;
				packet->app_payload.crc = app_crc(app_ptr);
				set_client(((node_packet_t*) *payload)->app_payload.id, server_data->client_fd);
				res = handle_broadcast(server_data->children, *payload, cmd_type);
			}
			break;
		default:
			custom_log_error("Unknown client request: %d", cmd_type);
			res = false;
			break;
	}

	free(*payload);

	return res;
}

static bool handle_node_request(void** payload, const uint8_t* buf, void* data) {
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
			client_fd = get_client_fd_by_id(((notify_t*) *payload)->app_msg_id);
			if (client_fd < 0) {
				custom_log_warn("Client fd is %d when sending return notify (id %d)", client_fd, ((notify_t*) *payload)->app_msg_id);
			}
			res = handle_notify(client_fd,  *payload);
			break;
		default:
			custom_log_error("Unsupported request");
			res = false;
			break;
	}

	free(*payload);

	return res;
}

static void init_clients(void) {
		uint8_t i;

		for (i = 0; i < MAX_CLIENT; i++) {
			clients[i].fd = -1;
		}
		client_num = 0;
}

static void set_client(uint16_t id, int32_t fd) {
	if (client_num == MAX_CLIENT) {
		client_num = 0;
	}

	clients[client_num].app_id = id;
	clients[client_num].fd = fd;
	client_num++;
}

static int32_t get_client_fd_by_id(uint16_t id) {
	uint8_t i;
	int32_t fd;

	for (i = 0; i < MAX_CLIENT; i++) {
		if (clients[i].app_id == id) {
			fd = clients[i].fd;
			clients[i].fd = -1;
			return fd;
		}
	}

	return -1;
}
