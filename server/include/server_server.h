#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "serving.h"
#include "settings.h"

typedef struct server {
	int32_t client_fd;
	struct node children[NODE_COUNT];
} server_t;

bool server_server_handle(server_t* server, const uint8_t* buf, int32_t conn_fd, void* data);
