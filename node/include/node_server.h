#pragma once

#include <stdint.h>         // for int32_t, uint8_t
#include <sys/types.h>      // for ssize_t

#include "custom_logger.h"  // for custom_log_error, custom_log_debug, custo...
#include "routing.h"        // for routing_table_t

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#define node_log_info(format, ...) custom_log_info("Node: " format, ##__VA_ARGS__)
#define node_log_error(format, ...) custom_log_error("Node: " format, ##__VA_ARGS__)
#define node_log_warn(format, ...) custom_log_warn("Node: " format, ##__VA_ARGS__)
#define node_log_debug(format, ...) custom_log_debug("Node: " format, ##__VA_ARGS__)

#ifdef __clang__
#pragma clang diagnostic pop
#endif

typedef struct node_server {
	routing_table_t routing;
	int32_t label;
} node_server_t;

int32_t node_server_handle_request(node_server_t* server, int32_t conn_fd, uint8_t* buf, ssize_t received_bytes, void* data);
