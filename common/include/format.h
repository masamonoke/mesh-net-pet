#pragma once

#include <stddef.h>

enum request_result {
	REQUEST_OK,
	REQUEST_ERR,
	REQUEST_UNKNOWN
};

__attribute__((nonnull(2)))
void format_sprint_result(enum request_result res, char buf[], size_t len);
