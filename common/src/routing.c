#include "routing.h"

#include <stdio.h>
#include <string.h>

#include "custom_logger.h"

int32_t routing_table_fill_default(routing_table_t* table, int32_t label) {
	size_t i;

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
