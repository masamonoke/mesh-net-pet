#pragma once

#ifndef SERVER_PORT
#define SERVER_PORT 999
#endif

#ifndef NODE_COUNT
#define NODE_COUNT 4
#endif

#define node_port(label) (uint16_t) (SERVER_PORT + (label) + 1)

// to generate names https://frightanic.com/goodies_content/docker-names.php
static const char* NODES_ALIASES[NODE_COUNT] = {
	#include "node_names.txt"
};

static const int ADJACENCY_MATRIX[][NODE_COUNT] = {
	#include "matrix.txt"
};
