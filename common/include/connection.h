#pragma once

#include <stdint.h>

__attribute__((warn_unused_result))
int32_t connection_socket_to_send(uint16_t port);

__attribute__((warn_unused_result))
int32_t connection_socket_to_listen(uint16_t port);
