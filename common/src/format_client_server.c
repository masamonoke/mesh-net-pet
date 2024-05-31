#include "format_client_server.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "control_utils.h"
#include "custom_logger.h"
#include "format.h"
#include "settings.h"

static void parse_message(enum request* request, void** payload, const uint8_t* buf);

static void create_message(enum request request, const void* payload, uint8_t* message, msg_len_type* msg_len);

void format_server_client_parse_message(enum request* req, void** payload, const void* buf) {
	parse_message(req, payload, (uint8_t*) buf);
}

void format_server_client_create_message(enum request req, const void* payload, uint8_t* buf, msg_len_type* len) {
	create_message(req, payload, buf, len);
}

static void parse_addr_payload(const uint8_t* buf, void* ret_payload);

static void parse_message(enum request* request, void** payload, const uint8_t* buf) {
	const uint8_t* p;
	enum request cmd;
	enum_ir tmp;

	*request = REQUEST_UNDEFINED;
	p = buf;

	memcpy(&tmp, p, sizeof_enum(cmd));
	cmd = (enum request) tmp;

	switch (cmd) {
		case REQUEST_SEND:
			*request = cmd;
			*payload = malloc(sizeof(struct send_to_node_ret_payload));
			format_parse_send(buf, (struct send_to_node_ret_payload*) *payload);
			break;
		case REQUEST_PING:
		case REQUEST_REVIVE_NODE:
		case REQUEST_KILL_NODE:
			*request = cmd;
			*payload = malloc(sizeof(uint8_t));
			parse_addr_payload(buf, *payload);
			break;
		case REQUEST_RESET:
			*request = cmd;
			break;
		case REQUEST_UNDEFINED:
			custom_log_error("Unknown client-server request");
			break;
		case REQUEST_BROADCAST:
			*request = cmd;
			*payload = malloc(sizeof(struct send_to_node_ret_payload));
			format_parse_broadcast(buf, (struct broadcast_payload*) *payload);
			break;
		default:
			not_implemented();
			break;
	}
}

static void create_message(enum request request, const void* payload, uint8_t* message, msg_len_type* msg_len) {
	uint8_t payload_len;
	uint8_t* p;
	enum request_sender sender;

	sender = REQUEST_SENDER_CLIENT;
	p = NULL;
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
				format_create_send(p, payload, message, msg_len, sender);
			} else {
				return;
			}
			break;
		case REQUEST_RESET:
			*msg_len = sizeof_enum(sender) + sizeof_enum(request) + sizeof(*msg_len);
			sender = REQUEST_SENDER_CLIENT;
			p = format_create_base(message, *msg_len, request, sender);
			break;
		case REQUEST_BROADCAST:
			if (payload) {
				format_create_broadcast(p, payload, message, msg_len, sender);
			}
			break;
		case REQUEST_UNICAST:
			break;
		default:
			not_implemented();
			break;
	}
}

static void parse_addr_payload(const uint8_t* buf, void* ret_payload) {
	const uint8_t* p;
	uint8_t* payload;

	payload = (uint8_t*) ret_payload;

	p = format_skip_base(buf);

	memcpy(payload, p, sizeof(*payload));
	p += sizeof(payload);
}
