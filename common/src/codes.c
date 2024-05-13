#include "codes.h"

char* codes_map_response_code(int32_t code) {
	switch (code) {
		case RESPONSE_OK:
			return "RESPONSE_OK";
		case RESPONSE_ERROR:
			return "RESPONSE_ERROR";
		default:
			return "RESPONSE_UNKNOWN";
	}
}
