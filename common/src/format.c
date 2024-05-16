#include <stdio.h>

#include "format.h"

void format_sprint_result(enum request_result res, char buf[], size_t len) {
	switch (res) {
		case REQUEST_OK:
			snprintf(buf, len, "[OK]: %d", res);
			break;
		case REQUEST_ERR:
			snprintf(buf, len, "[ERR]: %d", res);
			break;
		case REQUEST_UNKNOWN:
			snprintf(buf, len, "[UNKNOWN]: %d", res);
			break;
	}
}
