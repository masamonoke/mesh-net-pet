#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "settings.h"

enum app_request {
	APP_REQUEST_KEY_EXCHANGE,
	APP_REQUEST_DELIVERY,
	APP_REQUEST_EXCHANGED_KEY,
	APP_REQUEST_BROADCAST
};

struct app_payload {
	enum app_request req_type;
	uint8_t addr_to;
	uint8_t addr_from;
	uint8_t key;
	uint8_t message[APP_MESSAGE_LEN];
	uint8_t message_len;
	uint16_t crc;
};

__attribute__((nonnull(2)))
void format_app_create_message(const struct app_payload* app_payload, uint8_t* p);

__attribute__((nonnull(1, 2)))
void format_app_parse_message(void* payload, const uint8_t* p);

__attribute__((nonnull(1)))
uint8_t format_app_message_len(struct app_payload* payload);
