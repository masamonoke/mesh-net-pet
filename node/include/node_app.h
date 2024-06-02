#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "format_app.h"
#include "settings.h"

typedef struct app {
	uint8_t node_addr;
	uint8_t app_addr;
} app_t;

__attribute__((nonnull(1)))
void node_app_fill_default(app_t* apps, uint8_t node_addr);

__attribute__((nonnull(1, 2), warn_unused_result))
bool node_app_handle_request(app_t* apps, struct app_payload* app_payload, uint8_t node_addr);

__attribute__((nonnull(1)))
void node_app_setup_delivery(struct app_payload* app_payload);

__attribute__((nonnull(1, 2), warn_unused_result))
bool node_app_save_key(app_t apps[APPS_COUNT], struct app_payload* app_payload, uint8_t addr_from);
