#pragma once

#include <stdint.h> // for int32_t
#include <stddef.h> // for size_t

typedef struct append_buf {
	char* b;
	size_t len;
} append_buf_t;

void append_string_init(append_buf_t* buf);

void append_string_append(append_buf_t* buf, const char* s, size_t len);

void append_string_destroy(append_buf_t* buf);
