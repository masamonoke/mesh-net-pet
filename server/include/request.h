#pragma once

#include <stdbool.h>    // for bool
#include <stdint.h>     // for int32_t, uint8_t
#include <sys/types.h>  // for ssize_t

#include "serving.h"    // for node
#include "settings.h"   // for NODE_COUNT

typedef struct server {
	int32_t client_fd;
	struct node children[NODE_COUNT];
} server_t;

bool request_handle(server_t* server, const uint8_t* buf, ssize_t received_bytes, int32_t conn_fd, void* data);
