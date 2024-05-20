#pragma once

#include <stdint.h>
#include <stddef.h>

#include "format.h"

struct node_send_payload {
	int32_t label_to;
};

__attribute__((nonnull(3, 4), warn_unused_result))
int32_t format_node_node_create_message(enum request req, const void* payload, uint8_t* buf, uint32_t* len);

__attribute__((nonnull(1, 2, 3), warn_unused_result))
int32_t format_node_node_parse_message(enum request* req, void** payload, const void* buf, size_t len);
