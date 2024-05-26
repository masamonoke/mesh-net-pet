#pragma once

#include <stdint.h>

#include "format_node_node.h"

int32_t node_essentials_get_conn(int32_t port);

int32_t node_essentials_notify_server(void);

int32_t node_essentials_broadcast(int32_t current_label, int32_t banned_label, struct node_route_payload* route_payload, bool stop_broadcast);
