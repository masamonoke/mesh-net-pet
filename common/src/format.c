#include "format.h"

#include <stdio.h>   // for snprintf
#include <string.h>  // for memcpy

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

enum request_sender format_define_sender(const uint8_t* buf) {
	const uint8_t* p;
	uint32_t len;
	enum request_sender sender;

	p = buf;
	p += sizeof(len); // skip message len
	p += sizeof(enum request); // skip cmd
	memcpy(&sender, p, sizeof(sender));

	return sender;
}

uint8_t* format_create_base(uint8_t* message, uint32_t msg_len, enum request cmd, enum request_sender sender) { // NOLINT
	uint8_t* p;

	p = message;
	memcpy(p, &msg_len, sizeof(msg_len));
	p += sizeof(msg_len);
	memcpy(p, &cmd, sizeof(cmd));
	p += sizeof(cmd);
	memcpy(p, &sender, sizeof(sender));
	p += sizeof(sender);

	return p;
}

uint8_t* format_skip_base(const uint8_t* message) {
	uint8_t* p;

	p = (uint8_t*) message;

	p += sizeof(uint32_t); // skip message len
	p += sizeof(enum request); // skip cmd
	p += sizeof(enum request_sender); // skip sender

	return p;
}

bool format_is_message_correct(const uint8_t* buf, size_t buf_len) {
	uint32_t msg_len;

	if (buf_len > sizeof(uint32_t)) {
		memcpy(&msg_len, buf, sizeof(uint32_t));
		if (buf_len != msg_len) {
			return false;
		}
	} else {
		return false;
	}

	return true;
}
