#include <stddef.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

#include "format_client_server.h"
#include "custom_logger.h"
#include "control_utils.h"

static int32_t parse_message(enum request_type_server_client* request, void** payload, const uint8_t* buf, size_t buf_len);

static int32_t create_message(enum request_type_server_client request, const void* payload, uint8_t* message, uint32_t* msg_len);

int32_t format_server_client_parse_message(enum request_type_server_client* req, void** payload, const void* buf, size_t len) {
	return parse_message(req, payload, (uint8_t*) buf, len);
}

int32_t format_server_client_create_message(enum request_type_server_client req, const void* payload, uint8_t* buf, uint32_t* len) {
	return create_message(req, payload, buf, len);
}

static int32_t parse_ping(const uint8_t* buf, void* ret_payload);

static int32_t parse_message(enum request_type_server_client* request, void** payload, const uint8_t* buf, size_t buf_len) {
	uint32_t message_len;
	const uint8_t* p;

	*request = REQUEST_TYPE_SERVER_CLIENT_UNDEFINED;
	p = buf;

	if (buf_len > sizeof(message_len)) {
		memcpy(&message_len, p, sizeof(message_len));
		p += sizeof(message_len);
	} else {
		custom_log_error("Incorrect message format: message length < %d", sizeof(message_len));
		return -1;
	}

	if (buf_len == message_len) {
		uint32_t cmd_len;
		uint32_t sender_len;
		char cmd[32];
		char sender_str[32];

		memcpy(&cmd_len, p, sizeof(cmd_len));
		p += sizeof(cmd_len);

		if (buf_len > cmd_len) {
			// all strings null terminated
			memcpy(cmd, p, cmd_len);
			p += cmd_len;

			memcpy(&sender_len, p, sizeof(sender_len));
			p += sizeof(sender_len);
			memcpy(sender_str, p, sender_len);
			p += sender_len;

			if (0 == strcmp(cmd, "send") && 0 == strcmp(sender_str, "client")) {
				*request = REQUEST_TYPE_SERVER_CLIENT_SEND_AS_NODE;
			} else if (0 == strcmp(cmd, "ping") && 0 == strcmp(sender_str, "client")) {
				*request = REQUEST_TYPE_SERVER_CLIENT_PING_NODE;
			} else {
				custom_log_debug("Unknown request: cmd=%s, sender=%s", cmd, sender_str);
			}
		} else {
			custom_log_error("Incorrect message format: message length < declared command length %d", message_len);
		}
	} else {
		custom_log_error("Incorrect message format: message length < declared message length %d", message_len);
		return - 1;
	}

	switch (*request) {
		case REQUEST_TYPE_SERVER_CLIENT_PING_NODE:
		case REQUEST_TYPE_SERVER_CLIENT_SEND_AS_NODE:
			*payload = malloc(sizeof(struct node_ping_ret_payload));
			parse_ping(buf, *payload);
			break;
		case REQUEST_TYPE_SERVER_CLIENT_UNDEFINED:
			return -1;
			break;
	}

	return 0;
}

static int32_t create_message(enum request_type_server_client request, const void* payload, uint8_t* message, uint32_t* msg_len) {
	uint32_t payload_len;
	uint32_t cmd_len;
	char* cmd;
	uint32_t sender_len;
	char* sender;
	uint8_t* p;

	switch (request) {
		case REQUEST_TYPE_SERVER_CLIENT_PING_NODE:
			{
				struct node_ping_ret_payload* ret_payload;
				uint32_t label;

				ret_payload = (struct node_ping_ret_payload*) payload;
				payload_len = sizeof(label);

				cmd = "ping";
				sender = "client";
				cmd_len = (uint32_t) strlen(cmd) + 1;
				sender_len = (uint32_t) strlen(sender) + 1;
				*msg_len = payload_len + cmd_len + sender_len + sizeof(cmd_len) + sizeof(*msg_len) + sizeof(sender_len);

				p = message;

				memcpy(p, msg_len, sizeof(*msg_len));
				p += sizeof(*msg_len);

				memcpy(p, &cmd_len, sizeof(cmd_len));
				p += sizeof(cmd_len);
				memcpy(p, cmd, cmd_len);
				p += cmd_len;
				memcpy(p, &sender_len, sizeof(sender_len));
				p += sizeof(sender_len);
				memcpy(p, sender, sender_len);
				p += sender_len;

				memcpy(p, &ret_payload->label, sizeof(ret_payload->label));
				p += sizeof(ret_payload->label);
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
	uint32_t len;
	struct node_ping_ret_payload* payload;

	payload = (struct node_ping_ret_payload*) ret_payload;

	p = buf;

	p += sizeof(len); // skip message len
	memcpy(&len, p, sizeof(len));
	p += sizeof(len); // skip cmd size and cmd itself
	p += len;
	memcpy(&len, p, sizeof(len));
	p += sizeof(len); // skip sender size and sender itself
	p += len;

	memcpy(&payload->label, p, sizeof(payload->label));
	p += sizeof(payload->label);

	return 0;
}
