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
		default:
			custom_log_error("Unknown result response");
			break;
	}
}

enum request_sender format_define_sender(const uint8_t* buf) {
	const uint8_t* p;
	enum request_sender sender;
	enum_ir tmp;

	p = buf;
	p += sizeof_enum(enum request); // skip cmd
	memcpy(&tmp, p, sizeof_enum(sender));
	sender = (enum request_sender) tmp;

	return sender;
}

uint8_t* format_create_base(uint8_t* message, msg_len_type msg_len, enum request cmd, enum request_sender sender) { // NOLINT
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

bool format_is_message_correct(size_t buf_len, msg_len_type msg_len) {

	if (buf_len > sizeof(msg_len)) {
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

void format_create_send(uint8_t* p, const void* payload, uint8_t* message, msg_len_type* msg_len, enum request_sender sender) {
	struct send_to_node_ret_payload* ret_payload;
	uint8_t payload_len;

	ret_payload = (struct send_to_node_ret_payload*) payload;

	payload_len = sizeof(ret_payload->addr_to) + sizeof(ret_payload->addr_from) + format_app_message_len(&ret_payload->app_payload);
	*msg_len = payload_len + sizeof_enum(REQUEST_SEND) + sizeof(*msg_len) + sizeof_enum(sender);

	p = format_create_base(message, *msg_len, REQUEST_SEND, sender);

	memcpy(p, &ret_payload->addr_from, sizeof(ret_payload->addr_from));
	p += sizeof(ret_payload->addr_from);
	memcpy(p, &ret_payload->addr_to, sizeof(ret_payload->addr_to));
	p += sizeof(ret_payload->addr_to);

	format_app_create_message(&ret_payload->app_payload, p);
	p += format_app_message_len(&ret_payload->app_payload);
}

void format_parse_send(const uint8_t* buf, struct send_to_node_ret_payload* payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	memcpy(&payload->addr_from, p, sizeof(payload->addr_from));
	p += sizeof(payload->addr_from);
	memcpy(&payload->addr_to, p, sizeof(payload->addr_to));
	p += sizeof(payload->addr_to);

	format_app_parse_message(&payload->app_payload, p);
	p += format_app_message_len(&payload->app_payload);
}

void format_create_broadcast(uint8_t* p, const void* payload, uint8_t* message, msg_len_type* msg_len, enum request_sender sender) {
	struct broadcast_payload* ret_payload;
	uint8_t payload_len;

	ret_payload = (struct broadcast_payload*) payload;

	payload_len = sizeof(ret_payload->addr_from) + sizeof(ret_payload->time_to_live) + sizeof(ret_payload->local_from) + format_app_message_len(&ret_payload->app_payload);
	*msg_len = payload_len + sizeof_enum(REQUEST_BROADCAST) + sizeof(*msg_len) + sizeof_enum(sender);

	p = format_create_base(message, *msg_len, REQUEST_BROADCAST, sender);

	memcpy(p, &ret_payload->addr_from, sizeof(ret_payload->addr_from));
	p += sizeof(ret_payload->addr_from);
	memcpy(p, &ret_payload->time_to_live, sizeof(ret_payload->time_to_live));
	p += sizeof(ret_payload->time_to_live);
	memcpy(p, &ret_payload->local_from, sizeof(ret_payload->local_from));
	p += sizeof(ret_payload->local_from);

	format_app_create_message(&ret_payload->app_payload, p);
}

void format_parse_broadcast(const uint8_t* buf, struct broadcast_payload* payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	memcpy(&payload->addr_from, p, sizeof(payload->addr_from));
	p += sizeof(payload->addr_from);
	memcpy(&payload->time_to_live, p, sizeof(payload->time_to_live));
	p += sizeof(payload->time_to_live);
	memcpy(&payload->local_from, p, sizeof(payload->local_from));
	p += sizeof(payload->local_from);

	format_app_parse_message(&payload->app_payload, p);
}
