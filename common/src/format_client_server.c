#include "format_client_server.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "control_utils.h"
#include "custom_logger.h"
#include "format.h"
#include "settings.h"

static int32_t parse_message(enum request* request, void** payload, const uint8_t* buf);

static int32_t create_message(enum request request, const void* payload, uint8_t* message, uint32_t* msg_len);

int32_t format_server_client_parse_message(enum request* req, void** payload, const void* buf, size_t len) {
	(void) len;
	return parse_message(req, payload, (uint8_t*) buf);
}

int32_t format_server_client_create_message(enum request req, const void* payload, uint8_t* buf, uint32_t* len) {
	return create_message(req, payload, buf, len);
}

static int32_t parse_label_payload(const uint8_t* buf, void* ret_payload);

static int32_t parse_send(const uint8_t* buf, struct send_to_node_ret_payload* ret_payload);

static int32_t parse_message(enum request* request, void** payload, const uint8_t* buf) {
	const uint8_t* p;
	enum request cmd;

	*request = REQUEST_UNDEFINED;
	p = buf;

	memcpy(&cmd, p, sizeof_enum(cmd));

	switch (cmd) {
		case REQUEST_SEND:
			*request = cmd;
			*payload = malloc(sizeof(struct send_to_node_ret_payload));
			parse_send(buf, *payload);
			break;
		case REQUEST_PING:
		case REQUEST_REVIVE_NODE:
		case REQUEST_KILL_NODE:
			*request = cmd;
			*payload = malloc(sizeof(int32_t));
			parse_label_payload(buf, *payload);
			break;
		case REQUEST_RESET:
			*request = cmd;
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
		case REQUEST_KILL_NODE:
		case REQUEST_REVIVE_NODE:
			{
				if (payload) {
					uint8_t* ret_payload;

					ret_payload = (uint8_t*) payload;
					payload_len = sizeof(*ret_payload);

					*msg_len = payload_len + sizeof_enum(request) + sizeof(*msg_len) + sizeof_enum(sender);
					p = format_create_base(message, *msg_len, request, sender);

					memcpy(p, ret_payload, sizeof(*ret_payload));
				}
			}
			break;
		case REQUEST_SEND:
			if (payload) {
				struct send_to_node_ret_payload* ret_payload;

				ret_payload = (struct send_to_node_ret_payload*) payload;

				payload_len = sizeof(ret_payload->label_to) + sizeof(ret_payload->label_from) + format_app_message_len(&ret_payload->app_payload);
				*msg_len = payload_len + sizeof_enum(request) + sizeof(*msg_len) + sizeof_enum(sender);
				p = format_create_base(message, *msg_len, request, sender);

				memcpy(p, &ret_payload->label_from, sizeof(ret_payload->label_from));
				p += sizeof(ret_payload->label_from);
				memcpy(p, &ret_payload->label_to, sizeof(ret_payload->label_to));
				p += sizeof(ret_payload->label_to);

				format_app_create_message(&ret_payload->app_payload, p);
			} else {
				return -1;
			}
			break;
		case REQUEST_RESET:
			*msg_len = sizeof_enum(sender) + sizeof_enum(request) + sizeof(*msg_len);
			sender = REQUEST_SENDER_CLIENT;
			p = format_create_base(message, *msg_len, request, sender);
			break;
		default:
			not_implemented();
			break;
	}


	return 0;
}

static int32_t parse_label_payload(const uint8_t* buf, void* ret_payload) {
	const uint8_t* p;
	uint8_t* payload;

	payload = (uint8_t*) ret_payload;

	p = format_skip_base(buf);

	memcpy(payload, p, sizeof(*payload));
	p += sizeof(payload);

	return 0;
}

static int32_t parse_send(const uint8_t* buf, struct send_to_node_ret_payload* ret_payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	memcpy(&ret_payload->label_from, p, sizeof(ret_payload->label_from));
	p += sizeof(ret_payload->label_from);
	memcpy(&ret_payload->label_to, p, sizeof(ret_payload->label_to));
	p += sizeof(ret_payload->label_to);

	format_app_parse_message(&ret_payload->app_payload, p);

	return 0;
}
