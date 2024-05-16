#pragma once

#ifndef SERVER_PORT
#define SERVER_PORT 999
#endif

#ifndef NODE_COUNT
#define NODE_COUNT 4
#endif

// to generate names https://frightanic.com/goodies_content/docker-names.php
static const char* nodes_aliases[NODE_COUNT] = {
	"loving_kare",
	"mad_fermat",
	"boring_almeida",
	"romantic_euclid"
};
