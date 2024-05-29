#pragma once

#include <stdint.h>

#ifndef SERVER_PORT
#define SERVER_PORT 999
#endif

#ifndef MATRIX_SIZE
#define MATRIX_SIZE 10
#endif

#ifndef NODE_COUNT
#define NODE_COUNT (MATRIX_SIZE * MATRIX_SIZE)
#endif

#ifndef APPS_COUNT
#define APPS_COUNT 4
#endif

// depends on MATRIX_SIZE
#ifndef TTL
#define TTL (MATRIX_SIZE * 2)
#endif

#define sizeof_enum(_) (sizeof(uint32_t))

#define node_port(label) (uint16_t) (SERVER_PORT + (label) + 1)

#define node_label(port) (port - SERVER_PORT - 1)
