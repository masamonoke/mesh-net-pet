#pragma once

#include <stdint.h>

int32_t connection_socket_to_send(uint16_t port);

int32_t connection_socket_to_listen(uint16_t port);
