#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "format_app.h"
#include "settings.h"

#define MAX_PAIR 20

struct pair {
	int8_t app_label;
	int8_t node_label;
	uint8_t key;
	bool exchanged;
};

typedef struct app {
	int8_t node_label;
	int8_t app_label;
	struct pair pairs[MAX_PAIR];
} app_t;

__attribute__((nonnull(1)))
void node_app_fill_default(app_t* apps, int8_t node_addr);

__attribute__((nonnull(1, 2), warn_unused_result))
bool node_app_handle_request(app_t* apps, struct app_payload* app_payload, int8_t node_label_from);

__attribute__((nonnull(1, 2)))
void node_app_setup_delivery(app_t apps[APPS_COUNT], struct app_payload* app_payload, int8_t label_to);

__attribute__((nonnull(1, 2), warn_unused_result))
bool node_app_save_key(app_t apps[APPS_COUNT], struct app_payload* app_payload, int8_t label_from);
