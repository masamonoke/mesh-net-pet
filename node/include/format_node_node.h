#pragma once

#include <stdint.h>
#include <stddef.h>

#include "format.h"
#include "settings.h"

struct node_send_payload {
	int32_t label_to;
};

struct node_route_payload {
	int32_t sender_label;
	int32_t receiver_label;
	int32_t local_sender_label;
	int32_t metric;
	int32_t time_to_live;
	uint32_t id;
};

__attribute__((nonnull(3, 4), warn_unused_result))
int32_t format_node_node_create_message(enum request req, const void* payload, uint8_t* buf, uint32_t* len);

__attribute__((nonnull(1, 2, 3), warn_unused_result))
int32_t format_node_node_parse_message(enum request* req, void** payload, const void* buf, size_t len);
