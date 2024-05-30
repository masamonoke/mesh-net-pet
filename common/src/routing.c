#include "routing.h"

#include <stdio.h>
#include <string.h>

#include "custom_logger.h"

void routing_table_fill_default(routing_table_t* table) {
	size_t i;

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		table->nodes[i].addr = UINT8_MAX;
		table->nodes[i].metric = 0;
	}

	table->len = 0;
}

uint8_t routing_next_addr(const routing_table_t* table, uint8_t dest_addr) {
	if (dest_addr < NODE_COUNT) {
		return table->nodes[dest_addr].addr;
	} else {
		return UINT8_MAX;
	}
}

routing_node_t routing_get(const routing_table_t* table, uint8_t dest_addr) {
	if (dest_addr < NODE_COUNT) {
		return table->nodes[dest_addr];
	} else {
		routing_node_t empty_node = {
			.addr = UINT8_MAX,
			.metric = 0
		};
		return empty_node;
	}
}

void routing_set_addr(routing_table_t* table, uint8_t dest_addr, uint8_t next_addr, int8_t metric) { // NOLINT
	if (dest_addr < NODE_COUNT) {
		table->nodes[dest_addr].addr = next_addr;
		table->nodes[dest_addr].metric = metric;
	}
}

void routing_del(routing_table_t* table, uint8_t dest_addr) {
	if (dest_addr < NODE_COUNT) {
		table->nodes[dest_addr].addr = UINT8_MAX;
		table->nodes[dest_addr].metric = 0;
	}
}
