#pragma once

#ifndef SERVER_PORT
#define SERVER_PORT 999
#endif

#ifndef MATRIX_SIZE
#define MATRIX_SIZE 3
#define NODE_COUNT (MATRIX_SIZE * MATRIX_SIZE)
#endif

#define sizeof_enum(_) (sizeof(uint32_t))

#define node_port(label) (uint16_t) (SERVER_PORT + (label) + 1)
