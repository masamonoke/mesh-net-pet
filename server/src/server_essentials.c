#include "server_essentials.h"

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "control_utils.h"

void run_node(uint8_t node_addr) {
	char node_addr_str[16];
	int32_t len;

	len = snprintf(node_addr_str, sizeof(node_addr_str), "%d", node_addr);
	if (len < 0 || (size_t) len > sizeof(node_addr_str)) {
		die("Failed to convert node_addr to str");
	}

	if (execl("../node/mesh_node", "mesh_node", node_addr_str, (char*) NULL)) {
		die("Failed to run node");
	}
}

