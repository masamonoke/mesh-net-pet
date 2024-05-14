#pragma once

#include <stdint.h>
#include <stddef.h>

enum request_type {
	REQUEST_TYPE_SEND_AS_NODE,
	REQUEST_TYPE_PING_NODE,
	REQUEST_TYPE_NODE_UPDATE,
	REQUEST_TYPE_UNDEFINED
};

enum format_op_type {
	FORMAT_CREATE,
	FORMAT_PARSE
};

struct node_upate_ret_payload {
	int32_t pid;
	int32_t port;
	char alias[32];
	int32_t label;
};

struct send_to_node_ret_payload {
	uint32_t label;
	uint32_t status;
	uint8_t* path;
};

struct node_ping_ret_payload {
	uint32_t label;
	uint32_t status;
};

int32_t format_create_message(enum request_type req, void* payload, uint8_t* buf, uint32_t* len);

int32_t format_parse_message(enum request_type* req, void** payload, const void* buf, size_t len);
