#include "routing.h"

#include <stdio.h>
#include <string.h>

#include "custom_logger.h"
#include "path.h"

int32_t routing_table_new(routing_table_t* table, int32_t label) {
	size_t i;
	int32_t paths[V][V];

	if (label < 0) {
		custom_log_error("Wrong label passed");
		return -1;
	}

	for (i = 0; i < MAX_ROUTES; i++) {
		table->nodes[i].dest_label = -1;
		table->nodes[i].label = -1;
		table->nodes[i].fd = -1;
		table->nodes[i].port = -1;
	}

	table->len = 0;

	path_find(ADJACENCY_MATRIX, label, NULL, paths);

	for (i = 0; i < V; i++) {
		if (i != (size_t) label) {
			table->nodes[table->len].dest_label = (int32_t) i;
			table->nodes[table->len].label = paths[i][1];
			table->nodes[table->len].port = node_port(i);
			table->len++;
		}
	}


	return 0;
}

ssize_t routing_table_get_dest_idx(const routing_table_t* table, int32_t label) {
	size_t i;
	for (i = 0; i < MAX_ROUTES; i++) {
		if (table->nodes[i].dest_label == label) {
			return (ssize_t) i;
		}
	}

	return -1;
}

void routing_table_print(routing_table_t* table, int32_t label) {
	size_t i;
	char buf[1024];
	char format[32];

	snprintf(format, sizeof(format), "\nrouting table for label %d\n", label);
	strcpy(buf, format);
	strcat(buf, "dest\tthrough\n");
	for (i = 0; i < MAX_ROUTES; i++) {
		if (table->nodes[i].dest_label != -1) {
			snprintf(format, sizeof(format), "%d\t%d\n", table->nodes[i].dest_label, table->nodes[i].label);
			strcat(buf, format);
		}
	}

	custom_log_debug("%s", buf);
}
