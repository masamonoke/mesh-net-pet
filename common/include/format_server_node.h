#pragma once

#include <stdint.h>
#include <stddef.h>

#include "format.h"

#define UPDATE_LEN sizeof(struct node_update_ret_payload)

#define NOTIFY_LEN sizeof(enum_ir)

struct node_update_ret_payload {
	int32_t pid;
	uint16_t port;
	uint8_t addr;
};

enum notify_type {
	NOTIFY_GOT_MESSAGE,
	NOTIFY_INVERES_COMPLETED,
};

__attribute__((nonnull(3, 4)))
void format_server_node_create_message(enum request req,  const void* payload, uint8_t* buf, msg_len_type* len);

__attribute__((nonnull(1, 2, 3)))
void format_server_node_parse_message(enum request* req, void** payload, const void* buf);
