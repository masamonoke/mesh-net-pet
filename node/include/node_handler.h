#pragma once

#include <stdint.h>

#include "format.h"
#include "routing.h"

int32_t handle_ping(int32_t conn_fd);


__attribute__((nonnull(3, 4), warn_unused_result))
int32_t handle_server_send(enum request cmd_type, int8_t label, const void* payload, const routing_table_t* routing);

__attribute__((nonnull(2, 3), warn_unused_result))
int32_t handle_node_send(int8_t label, const void* payload, const routing_table_t* routing);

__attribute__((nonnull(1, 3), warn_unused_result))
int32_t handle_node_route_direct(routing_table_t* routing, int8_t server_label, void* payload);

__attribute__((nonnull(1), warn_unused_result))
int32_t handle_node_route_inverse(routing_table_t* routing, void* payload, int8_t server_label);

void handle_stop_broadcast(void);

void handle_reset_broadcast_status(void);
