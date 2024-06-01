#pragma once

#include <stdint.h>
#include <stddef.h>

#include "format.h"

#define UPDATE_LEN sizeof(node_update_t)

#define NOTIFY_LEN sizeof(enum_ir)

typedef struct node_update_payload {
	int32_t pid;
	uint16_t port;
	uint8_t addr;
} node_update_t;

enum __attribute__((packed, aligned(1))) notify_type {
	NOTIFY_GOT_MESSAGE,
	NOTIFY_INVERES_COMPLETED,
	NOTIFY_FAIL,
	NOTIFY_UNICAST_HANDLED
};

__attribute__((nonnull(3, 4)))
void format_server_node_create_message(enum request req,  const void* payload, uint8_t* buf, msg_len_type* len);

__attribute__((nonnull(1, 2, 3)))
void format_server_node_parse_message(enum request* req, void** payload, const void* buf);
