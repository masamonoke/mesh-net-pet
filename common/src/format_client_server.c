#include "format_client_server.h"

#include <stddef.h>         // for size_t
#include <stdlib.h>         // for malloc
#include <string.h>         // for memcpy

#include "control_utils.h"  // for not_implemented
#include "custom_logger.h"  // for custom_log_error
#include "format.h"         // for request, format_create_base, format_skip_...

static int32_t parse_message(enum request* request, void** payload, const uint8_t* buf, size_t buf_len);

static int32_t create_message(enum request request, const void* payload, uint8_t* message, uint32_t* msg_len);

int32_t format_server_client_parse_message(enum request* req, void** payload, const void* buf, size_t len) {
	return parse_message(req, payload, (uint8_t*) buf, len);
}

int32_t format_server_client_create_message(enum request req, const void* payload, uint8_t* buf, uint32_t* len) {
	return create_message(req, payload, buf, len);
}

static int32_t parse_ping(const uint8_t* buf, void* ret_payload);

static int32_t parse_send(const uint8_t* buf, struct send_to_node_ret_payload* ret_payload);

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
			*request = cmd;
			*payload = malloc(sizeof(struct send_to_node_ret_payload));
			parse_send(buf, *payload);
			break;
		case REQUEST_PING:
			*request = cmd;
			*payload = malloc(sizeof(struct node_ping_ret_payload));
			parse_ping(buf, *payload);
			break;
		default:
			custom_log_error("Unknown client-server request");
	}

	return 0;
}

static int32_t create_message(enum request request, const void* payload, uint8_t* message, uint32_t* msg_len) {
	uint32_t payload_len;
	uint8_t* p;
	enum request_sender sender;

	sender = REQUEST_SENDER_CLIENT;

	switch (request) {
		case REQUEST_PING:
			{
				struct node_ping_ret_payload* ret_payload;
				uint32_t label;

				ret_payload = (struct node_ping_ret_payload*) payload;
				payload_len = sizeof(label);

				*msg_len = payload_len + sizeof(request) + sizeof(*msg_len) + sizeof(sender);

				p = format_create_base(message, *msg_len, request, sender);

				memcpy(p, &ret_payload->label, sizeof(ret_payload->label));
				p += sizeof(ret_payload->label);
			}
			break;
		case REQUEST_SEND:
			if (payload) {
				struct send_to_node_ret_payload* ret_payload;

				ret_payload = (struct send_to_node_ret_payload*) payload;
				payload_len = sizeof(ret_payload->label_to) + sizeof(ret_payload->label_from);
				*msg_len = payload_len + sizeof(request) + sizeof(*msg_len) + sizeof(sender);

				p = format_create_base(message, *msg_len, request, sender);

				memcpy(p, &ret_payload->label_from, sizeof(ret_payload->label_from));
				p += sizeof(ret_payload->label_from);
				memcpy(p, &ret_payload->label_to, sizeof(ret_payload->label_to));
				p += sizeof(ret_payload->label_to);
			} else {
				return -1;
			}
			break;
		default:
			not_implemented();
			break;
	}


	return 0;
}

static int32_t parse_ping(const uint8_t* buf, void* ret_payload) {
	const uint8_t* p;
	struct node_ping_ret_payload* payload;

	payload = (struct node_ping_ret_payload*) ret_payload;

	p = format_skip_base(buf);

	memcpy(&payload->label, p, sizeof(payload->label));
	p += sizeof(payload->label);

	return 0;
}

static int32_t parse_send(const uint8_t* buf, struct send_to_node_ret_payload* ret_payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	memcpy(&ret_payload->label_from, p, sizeof(ret_payload->label_from));
	p += sizeof(ret_payload->label_from);
	memcpy(&ret_payload->label_to, p, sizeof(ret_payload->label_from));

	return 0;
}
