#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "format_app.h"
#include "settings.h"

#define MAX_PAIR 20

struct pair {
	uint8_t app_addr;
	uint8_t node_addr;
	uint8_t key;
	bool exchanged;
};

typedef struct app {
	uint8_t node_addr;
	uint8_t app_addr;
	struct pair pairs[MAX_PAIR];
} app_t;

__attribute__((nonnull(1)))
void node_app_fill_default(app_t* apps, uint8_t node_addr);

__attribute__((nonnull(1, 2), warn_unused_result))
bool node_app_handle_request(app_t* apps, struct app_payload* app_payload, uint8_t node_addr_from);

__attribute__((nonnull(1, 2)))
void node_app_setup_delivery(app_t apps[APPS_COUNT], struct app_payload* app_payload, uint8_t addr_to);

__attribute__((nonnull(1, 2), warn_unused_result))
bool node_app_save_key(app_t apps[APPS_COUNT], struct app_payload* app_payload, uint8_t addr_from);
