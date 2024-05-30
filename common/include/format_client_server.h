#pragma once

#include <stdint.h>
#include <stddef.h>

#include "format.h"
#include "format_app.h"

__attribute__((nonnull(3, 4)))
void format_server_client_create_message(enum request req, const void* payload, uint8_t* buf, msg_len_type* len);

__attribute__((nonnull(1, 2, 3)))
void format_server_client_parse_message(enum request* req, void** payload, const void* buf);
