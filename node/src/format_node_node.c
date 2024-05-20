#include <memory.h>
#include <stdlib.h>

#include "format_node_node.h"
#include "custom_logger.h"
#include "control_utils.h"

static int32_t parse_message(enum request* request, void** payload, const uint8_t* buf, size_t buf_len);

static int32_t create_message(enum request request, const void* payload, uint8_t* message, uint32_t* msg_len);

int32_t format_node_node_parse_message(enum request* req, void** payload, const void* buf, size_t len) {
	return parse_message(req, payload, (uint8_t*) buf, len);
}

int32_t format_node_node_create_message(enum request req, const void* payload, uint8_t* buf, uint32_t* len) {
	return create_message(req, payload, buf, len);
}

static int32_t set_send_payload(const uint8_t* buf, struct node_send_payload* payload);

static int32_t parse_message(enum request* request, void** payload, const uint8_t* buf, size_t buf_len) {
	const uint8_t* p;
	enum request cmd;

	*request = REQUEST_UNDEFINED;

	p = buf;

	if (!format_is_message_correct(buf, buf_len)) {
		return -1;
	}

	p += sizeof(uint32_t); // skip msg_len

	memcpy(&cmd, p, sizeof(cmd));

	switch (cmd) {
		case REQUEST_SEND:
			{
				*request = cmd;
				*payload = malloc(sizeof(struct node_send_payload*));
				set_send_payload(buf, (struct node_send_payload*) *payload);
			}
			break;
		default:
			custom_log_error("Unknown node-node request");
			break;
	}

	return 0;
}

static int32_t create_message(enum request request, const void* payload, uint8_t* message, uint32_t* msg_len) {
	uint32_t payload_len;
	uint8_t* p;
	enum request_sender sender;

	switch (request) {
		case REQUEST_SEND:
			{
				struct node_send_payload* send_payload;
				send_payload = (struct node_send_payload*) payload;
				payload_len = sizeof(send_payload->label_to);
				sender = REQUEST_SENDER_NODE;

				*msg_len = payload_len + sizeof(request) + sizeof(*msg_len) + sizeof(sender);

				p = format_create_base(message, *msg_len, request, sender);

				memcpy(p, &send_payload->label_to, sizeof(send_payload->label_to));
			}
			break;
		default:
			custom_log_error("Not supported node-node command");
			break;
	}

	return 0;
}

static int32_t set_send_payload(const uint8_t* buf, struct node_send_payload* payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	// parse payload
	memcpy(&payload->label_to, p, sizeof(payload->label_to));

	return 0;
}
