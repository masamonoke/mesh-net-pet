#include "routing.h"

#include <stdio.h>
#include <string.h>

#include "custom_logger.h"

int32_t routing_table_fill_default(routing_table_t* table) {
	size_t i;

	for (i = 0; i < (size_t) NODE_COUNT; i++) {
		table->nodes[i].label = -1;
		table->nodes[i].metric = 0;
	}

	table->len = 0;

	return 0;
}

int32_t routing_next_label(const routing_table_t* table, int32_t dest_label) {
	if (dest_label < NODE_COUNT) {
		return table->nodes[dest_label].label;
	} else {
		return -1;
	}
}

routing_node_t routing_get(const routing_table_t* table, int32_t dest_label) {
	if (dest_label < NODE_COUNT) {
		return table->nodes[dest_label];
	} else {
		routing_node_t empty_node = {
			.label = -1,
			.metric = 0
		};
		return empty_node;
	}
}

void routing_set_label(routing_table_t* table, int32_t dest_label, int32_t next_label, int32_t metric) { // NOLINT
	if (dest_label < NODE_COUNT) {
		table->nodes[dest_label].label = next_label;
		table->nodes[dest_label].metric = metric;
	}
}

void routing_del(routing_table_t* table, int32_t dest_label) {
	if (dest_label < NODE_COUNT) {
		table->nodes[dest_label].label = -1;
		table->nodes[dest_label].metric = 0;
	}
}
