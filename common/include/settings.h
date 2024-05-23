#pragma once

#ifndef SERVER_PORT
#define SERVER_PORT 999
#endif

#ifndef NODE_COUNT
#define NODE_COUNT 4
#endif

#define node_port(label) (uint16_t) (SERVER_PORT + (label) + 1)
