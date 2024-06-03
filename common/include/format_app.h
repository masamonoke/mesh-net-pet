#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "settings.h"

#define MAX_APP_LEN (sizeof_enum(req_type) + sizeof(uint8_t) * 3 + sizeof(uint16_t) * 2 + APP_MESSAGE_LEN)

#define app_crc(app_ptr) crc16((uint8_t*) (app_ptr), format_app_message_len((app_ptr)) - sizeof((app_ptr)->crc))

enum __attribute__((packed, aligned(1))) app_request {
	APP_REQUEST_DELIVERY,
	APP_REQUEST_BROADCAST,
	APP_REQUEST_UNICAST,
};

struct __attribute__((__packed__)) app_payload {
	enum app_request req_type;
	uint8_t addr_from;
	uint8_t addr_to;
	uint16_t id;
	uint8_t message_len;
	uint8_t message[APP_MESSAGE_LEN];
	uint16_t crc;
};

__attribute__((nonnull(2)))
void format_app_create_message(const struct app_payload* app_payload, uint8_t* p);

__attribute__((nonnull(1, 2)))
void format_app_parse_message(void* payload, const uint8_t* p);

__attribute__((nonnull(1)))
uint8_t format_app_message_len(struct app_payload* payload);
