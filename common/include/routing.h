#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>

#include "settings.h"

typedef struct routing_node {
	// how to get to
	uint8_t label;
	int8_t metric;
} routing_node_t;

typedef struct routing_table {
	routing_node_t nodes[NODE_COUNT];
	size_t len;
} routing_table_t;

__attribute__((nonnull(1), warn_unused_result))
int8_t routing_table_fill_default(routing_table_t* table);

__attribute__((nonnull(1), warn_unused_result))
uint8_t routing_next_label(const routing_table_t* table, uint8_t dest_label);

__attribute__((nonnull(1), warn_unused_result))
routing_node_t routing_get(const routing_table_t* table, uint8_t dest_label);

__attribute__((nonnull(1)))
void routing_del(routing_table_t* table, uint8_t dest_label);

__attribute__((nonnull(1)))
void routing_set_label(routing_table_t* table, uint8_t dest_label, uint8_t next_label, int8_t metric);
