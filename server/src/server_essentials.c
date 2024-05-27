#include "server_essentials.h"

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "control_utils.h"

void run_node(int32_t node_label) {
	char node_label_str[16];
	int32_t len;

	len = snprintf(node_label_str, sizeof(node_label_str), "%d", node_label);
	if (len < 0 || (size_t) len > sizeof(node_label_str)) {
		die("Failed to convert node_label to str");
	}

	if (execl("../node/mesh_node", "mesh_node", node_label_str, (char*) NULL)) {
		die("Failed to run node");
	}
}

