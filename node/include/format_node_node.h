#pragma once

#include <stdint.h>
#include <stddef.h>

#include "format.h"
#include "format_app.h"

#define ROUTE_INVERSE_LEN sizeof(struct node_route_inverse_payload)

#define ROUTE_DIRECT_LEN (sizeof(uint8_t) * 3 + sizeof(uint8_t) * 2 + sizeof(uint16_t) + MAX_APP_LEN)

struct node_route_direct_payload {
	uint8_t sender_addr;
	uint8_t receiver_addr;
	uint8_t local_sender_addr;
	int8_t metric;
	int8_t time_to_live;
	uint16_t id;
	struct app_payload app_payload;
};

struct node_route_inverse_payload {
	uint8_t sender_addr;
	uint8_t receiver_addr;
	uint8_t local_sender_addr;
	int8_t metric;
	int8_t time_to_live;
	uint16_t id;
};

__attribute__((nonnull(3, 4)))
void format_node_node_create_message(enum request req, const void* payload, uint8_t* buf, msg_len_type* len);

__attribute__((nonnull(1, 2, 3)))
void format_node_node_parse_message(enum request* req, void** payload, const void* buf);
