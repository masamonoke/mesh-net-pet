#include "append_string.h"

#include <stdlib.h>
#include <memory.h>

void append_string_init(append_buf_t* buf) {
	buf->b = NULL;
	buf->len = 0;
}

void append_string_append(append_buf_t* buf, const char* s, size_t len) {
	char* new;

	new = realloc(buf->b, buf->len + len + 1);

	if (new == NULL) {
		return;
	}

	memcpy(&new[buf->len], s, len);
	buf->b = new;
	buf->b[len] = '\0';
	buf->len += len + 1;

}

void append_string_destroy(append_buf_t* buf) {
	free(buf->b);
}

