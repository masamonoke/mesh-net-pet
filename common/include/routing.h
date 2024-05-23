#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#include "settings.h"

typedef struct routing_node {
	// destination
	int32_t dest_label;
	// how to get to
	int32_t fd;
	int32_t label;
	int32_t port;
} routing_node_t;

#define MAX_ROUTES (NODE_COUNT - 1)

typedef struct routing_table {
	routing_node_t nodes[MAX_ROUTES];
	size_t len;
} routing_table_t;

__attribute__((nonnull(1), warn_unused_result))
int32_t routing_table_fill_default(routing_table_t* table, int32_t label);

__attribute__((nonnull(1), warn_unused_result))
ssize_t routing_table_get_dest_idx(const routing_table_t* table, int32_t label);

__attribute__((nonnull(1)))
void routing_table_print(routing_table_t* table, int32_t label);
