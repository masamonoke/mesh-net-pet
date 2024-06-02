#pragma once

#include <stdint.h>

#include "format.h"
#include "routing.h"
#include "node_app.h"

__attribute__((warn_unused_result))
bool handle_ping(int32_t conn_fd);

__attribute__((nonnull(3, 4), warn_unused_result))
bool handle_server_send(enum request cmd_type, uint8_t addr, const void* payload, const routing_table_t* routing, app_t apps[APPS_COUNT]);

__attribute__((nonnull(2, 3), warn_unused_result))
bool handle_node_send(uint8_t addr, const void* payload, const routing_table_t* routing, app_t apps[APPS_COUNT]);

__attribute__((nonnull(1, 3), warn_unused_result))
bool handle_node_route_direct(routing_table_t* routing, uint8_t server_addr, void* payload, app_t apps[APPS_COUNT]);

__attribute__((nonnull(1), warn_unused_result))
bool handle_node_route_inverse(routing_table_t* routing, void* payload, uint8_t server_addr);

__attribute__((nonnull(1)))
void handle_broadcast(broadcast_t* broadcast_payload);

void handle_reset_broadcast_status(void);
