#pragma once

#include "stdint.h"

enum response_codes {
	RESPONSE_OK = 0,
	RESPONSE_ERROR = 1,
	RESPONSE_UNKNOWN
};

char* codes_map_response_code(int32_t code);
