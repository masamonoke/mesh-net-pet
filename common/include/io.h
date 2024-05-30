#pragma once

#include <stdint.h>
#include <stddef.h>

#include "format.h"

__attribute__((nonnull(2), warn_unused_result))
bool io_read_all(int32_t fd, uint8_t* buf_mut, msg_len_type n, int16_t* bytes_received);

__attribute__((nonnull(2), warn_unused_result))
bool io_write_all(int32_t fd, const uint8_t* buf, msg_len_type n);
