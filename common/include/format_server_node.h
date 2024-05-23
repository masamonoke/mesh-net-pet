#pragma once

#include <stdint.h>
#include <stddef.h>

#include "format.h"

struct node_update_ret_payload {
	int32_t pid;
	int32_t port;
	int32_t label;
};

enum notify_type {
	NOTIFY_GOT_MESSAGE
};

struct node_notify_payload {
	enum notify_type notify_type;
};

__attribute__((nonnull(3, 4), warn_unused_result))
int32_t format_server_node_create_message(enum request req,  const void* payload, uint8_t* buf, uint32_t* len);

__attribute__((nonnull(1, 2, 3), warn_unused_result))
int32_t format_server_node_parse_message(enum request* req, void** payload, const void* buf, size_t len);
