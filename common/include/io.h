#pragma once

#include <stdint.h>
#include <stddef.h>

__attribute__((nonnull(2), warn_unused_result))
int32_t io_read_all(int32_t fd, char* buf_mut, size_t n, size_t* bytes_received);

__attribute__((nonnull(2), warn_unused_result))
int32_t io_write_all(int32_t fd, const char* buf, size_t n);

__attribute__((nonnull, warn_unused_result))
int32_t io_read_file(uint8_t** bytes_to_send, const char* filename);

__attribute__((nonnull, warn_unused_result))
int32_t io_read_file_binary(uint8_t** bytes_to_send, const char* filename, uint32_t* len);

__attribute__((nonnull(1, 2), warn_unused_result))
int32_t io_write_file_binary(uint8_t** bytes, const char* filename, uint32_t len);
