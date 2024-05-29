#pragma once

#include <stdint.h>
#include <sys/types.h>

#include "routing.h"
#include "node_app.h"

typedef struct node_server {
	routing_table_t routing;
	uint8_t label;
	app_t apps[APPS_COUNT];
} node_server_t;

int32_t node_server_handle_request(node_server_t* server, int32_t conn_fd, uint8_t* buf, ssize_t received_bytes, void* data);
