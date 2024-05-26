#include "format.h"
#include "settings.h"
#include "custom_logger.h"

#include <stdio.h>
#include <string.h>

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
	enum request_sender sender;

	p = buf;
	p += sizeof_enum(enum request); // skip cmd
	memcpy(&sender, p, sizeof_enum(sender));

	return sender;
}

uint8_t* format_create_base(uint8_t* message, uint32_t msg_len, enum request cmd, enum request_sender sender) { // NOLINT
	uint8_t* p;

	p = message;
	memcpy(p, &msg_len, sizeof(msg_len));
	p += sizeof(msg_len);
	memcpy(p, &cmd, sizeof_enum(cmd));
	p += sizeof_enum(cmd);
	memcpy(p, &sender, sizeof_enum(sender));
	p += sizeof_enum(sender);

	return p;
}

uint8_t* format_skip_base(const uint8_t* message) {
	uint8_t* p;

	p = (uint8_t*) message;

	p += sizeof_enum(enum request); // skip cmd
	p += sizeof_enum(enum request_sender); // skip sender

	return p;
}

bool format_is_message_correct(size_t buf_len, uint32_t msg_len) {

	if (buf_len > sizeof(uint32_t)) {
		if (buf_len != msg_len) {
			custom_log_error("Incorrect message format: buffer length (%d) != delcared message length (%d)", buf_len, msg_len);
			return false;
		}
	} else {
		custom_log_error("Incorrect message format: buffer length (%d) < size of uint32_t", buf_len);
		return false;
	}

	return true;
}
