#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

enum app_request {
	APP_REQUEST_KEY_EXCHANGE,
	APP_REQUEST_DELIVERY,
	APP_REQUEST_EXCHANGED_KEY
};

struct app_payload {
	enum app_request req_type;
	int8_t label_to;
	int8_t label_from;
	uint8_t key;
	uint8_t message[150];
	uint8_t message_len;
};

__attribute__((nonnull(2)))
void format_app_create_message(const struct app_payload* app_payload, uint8_t* p);

__attribute__((nonnull(1, 2)))
void format_app_parse_message(void* payload, const uint8_t* p);

uint32_t format_app_message_len(struct app_payload* payload);
