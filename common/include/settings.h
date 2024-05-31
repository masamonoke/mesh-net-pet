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

#ifndef MAX_MSG_LEN
#define MAX_MSG_LEN 256
#endif

#ifndef APP_MESSAGE_LEN
#define APP_MESSAGE_LEN 150
#endif

#ifndef BROADCAST_RADIUS
#define BROADCAST_RADIUS 3
#endif

#define enum_ir uint8_t

// argument needs for readability only
#define sizeof_enum(enum_name_or_var) sizeof(enum_ir)

#define node_port(addr) (uint16_t) (SERVER_PORT + (addr) + 1)

#define node_addr(port) (port - SERVER_PORT - 1)
