#pragma once

#include <stdint.h>
#include <stddef.h>

#include "format.h"

struct send_to_node_ret_payload {
	int32_t label_to;
	int32_t label_from;
};

struct node_ping_ret_payload {
	int32_t label;
};

__attribute__((nonnull(3, 4), warn_unused_result))
int32_t format_server_client_create_message(enum request req, const void* payload, uint8_t* buf, uint32_t* len);

__attribute__((nonnull(1, 2, 3), warn_unused_result))
int32_t format_server_client_parse_message(enum request* req, void** payload, const void* buf, size_t len);
