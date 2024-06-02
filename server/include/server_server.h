#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "serving.h"
#include "settings.h"

#define MAX_CLIENT 4

typedef struct client {
	int32_t fd;
	uint16_t app_id;
} client_t;

typedef struct server {
	int32_t client_fd;
	client_t clients[MAX_CLIENT];
	uint8_t client_num;
	struct node children[NODE_COUNT];
} server_t;

__attribute__((nonnull(1, 2)))
bool server_server_handle(server_t* server, const uint8_t* buf, int32_t conn_fd, void* data);
