#pragma once

#include <stdint.h>
#include <stddef.h>

enum request_type_server_client {
	REQUEST_TYPE_SERVER_CLIENT_SEND_AS_NODE,
	REQUEST_TYPE_SERVER_CLIENT_PING_NODE,
	REQUEST_TYPE_SERVER_CLIENT_UNDEFINED
};

struct send_to_node_ret_payload {
	int32_t label;
	uint8_t* path;
	uint32_t node_count;
};

struct node_ping_ret_payload {
	int32_t label;
};

int32_t format_server_client_create_message(enum request_type_server_client req, const void* payload, uint8_t* buf, uint32_t* len);

int32_t format_server_client_parse_message(enum request_type_server_client* req, void** payload, const void* buf, size_t len);
