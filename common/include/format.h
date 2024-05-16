#pragma once

#include <stddef.h>

enum request_result {
	REQUEST_OK,
	REQUEST_ERR,
	REQUEST_UNKNOWN
};

void format_sprint_result(enum request_result res, char buf[], size_t len);
