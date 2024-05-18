#pragma once

#include <stdint.h>
#include <stddef.h>

enum request_type_server_node {
	REQUEST_TYPE_SERVER_NODE_UPDATE,
	REQUEST_TYPE_SERVER_NODE_PING,
	REQUEST_TYPE_SERVER_NODE_UNDEFINED
};

struct node_update_ret_payload {
	int32_t pid;
	int32_t port;
	char alias[32];
	int32_t label;
};

__attribute__((nonnull(3, 4)))
int32_t format_server_node_create_message(enum request_type_server_node req, const void* payload, uint8_t* buf, uint32_t* len);

__attribute__((nonnull(1, 2, 3)))
int32_t format_server_node_parse_message(enum request_type_server_node* req, void** payload, const void* buf, size_t len);
