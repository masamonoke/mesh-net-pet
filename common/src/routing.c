#include "routing.h"

#include <stdio.h>
#include <string.h>

#include "custom_logger.h"

int8_t routing_table_fill_default(routing_table_t* table) {
	size_t i;

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		table->nodes[i].label = UINT8_MAX;
		table->nodes[i].metric = 0;
	}

	table->len = 0;

	return 0;
}

uint8_t routing_next_label(const routing_table_t* table, uint8_t dest_label) {
	if (dest_label < NODE_COUNT) {
		return table->nodes[dest_label].label;
	} else {
		return UINT8_MAX;
	}
}

routing_node_t routing_get(const routing_table_t* table, uint8_t dest_label) {
	if (dest_label < NODE_COUNT) {
		return table->nodes[dest_label];
	} else {
		routing_node_t empty_node = {
			.label = UINT8_MAX,
			.metric = 0
		};
		return empty_node;
	}
}

void routing_set_label(routing_table_t* table, uint8_t dest_label, uint8_t next_label, int8_t metric) { // NOLINT
	if (dest_label < NODE_COUNT) {
		table->nodes[dest_label].label = next_label;
		table->nodes[dest_label].metric = metric;
	}
}

void routing_del(routing_table_t* table, uint8_t dest_label) {
	if (dest_label < NODE_COUNT) {
		table->nodes[dest_label].label = UINT8_MAX;
		table->nodes[dest_label].metric = 0;
	}
}
