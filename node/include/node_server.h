#pragma once

#include <stdint.h>
#include <sys/types.h>

#include "routing.h"

typedef struct node_server {
	routing_table_t routing;
	int8_t label;
} node_server_t;

int32_t node_server_handle_request(node_server_t* server, int32_t conn_fd, uint8_t* buf, ssize_t received_bytes, void* data);
