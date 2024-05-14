#include <stddef.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>

#include "format.h"
#include "custom_logger.h"
#include "control_utils.h"

static int32_t parse_message(enum request_type* request, void** payload, const uint8_t* buf, size_t buf_len);

static int32_t create_message(enum request_type request, void* payload, uint8_t* message, uint32_t* msg_len);

int32_t format_parse_message(enum request_type* req, void** payload, const void* buf, size_t len) {
	return parse_message(req, payload, (uint8_t*) buf, len);
}

int32_t format_create_message(enum request_type req, void* payload, uint8_t* buf, uint32_t* len) {
	return create_message(req, payload, buf, len);
}

static int32_t handle_node_update(const uint8_t* buf, void* ret_payload);

static int32_t parse_message(enum request_type* request, void** payload, const uint8_t* buf, size_t buf_len) {
	uint32_t message_len;
	const uint8_t* p;

	*request = REQUEST_TYPE_UNDEFINED;
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
				*request = REQUEST_TYPE_SEND_AS_NODE;
			} else if (0 == strcmp(cmd, "update_node") && 0 == strcmp(sender_str, "node")) {
				*request = REQUEST_TYPE_NODE_UPDATE;
			} else {
				custom_log_error("Unknown request format: cmd = %s", cmd);
			}
		} else {
			custom_log_error("Incorrect message format: message length < declared command length %d", message_len);
		}
	} else {
		custom_log_error("Incorrect message format: message length < declared message length %d", message_len);
		return - 1;
	}

	switch (*request) {
		case REQUEST_TYPE_PING_NODE:
		case REQUEST_TYPE_SEND_AS_NODE:
			not_implemented();
			break;
		case REQUEST_TYPE_NODE_UPDATE:
			*payload = malloc(sizeof(struct node_upate_ret_payload));
			handle_node_update(buf, *payload);
			break;
		case REQUEST_TYPE_UNDEFINED:
			custom_log_error("Unknown request type");
	}

	return 0;
}

static int32_t handle_node_update(const uint8_t* buf, void* ret_payload) {
	const uint8_t* p;
	uint32_t len;
	char str[32];
	struct node_upate_ret_payload* payload;

	payload = (struct node_upate_ret_payload*) ret_payload;

	p = buf;

	p += sizeof(len); // skip message len
	memcpy(&len, p, sizeof(len));
	p += sizeof(len); // skip cmd size and cmd itself
	p += len;
	memcpy(&len, p, sizeof(len));
	p += sizeof(len); // skip sender size and sender itself
	p += len;

	// parse payload
	memcpy(&payload->port, p, sizeof(payload->port));
	p += sizeof(payload->port);
	memcpy(&len, p, sizeof(len));
	p += sizeof(len);
	memcpy(str, p, len);
	p += len;
	str[len] = 0;
	strcpy(payload->alias, str);
	memcpy(&payload->label, p, sizeof(payload->label));
	p += sizeof(payload->label);
	memcpy(&payload->pid, p, sizeof(payload->pid));

	return 0;
}

static int32_t create_message(enum request_type request, void* payload, uint8_t* message, uint32_t* msg_len) {
	uint32_t alias_len;
	uint32_t payload_len;
	int32_t pid;
	uint8_t* p;
	uint32_t cmd_len;
	char* sender;
	char* cmd;
	uint32_t sender_len;

	switch (request) {
		case REQUEST_TYPE_NODE_UPDATE:
			{
				struct node_upate_ret_payload* update_payload;
				cmd = "update_node";

				update_payload = (struct node_upate_ret_payload*) payload;
				alias_len = (uint32_t) strlen(update_payload->alias);
				pid = getpid();
				payload_len = sizeof(update_payload->port) + sizeof(alias_len) + alias_len + sizeof(update_payload->label) + sizeof(pid);
				sender = "node";

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

				// payload
				memcpy(p, &update_payload->port, sizeof(update_payload->port));
				p += sizeof(update_payload->port);
				memcpy(p, &alias_len, sizeof(alias_len));
				p += sizeof(alias_len);
				memcpy(p, update_payload->alias, alias_len);
				p += alias_len;
				memcpy(p, &update_payload->label, sizeof(update_payload->label));
				p += sizeof(update_payload->label);
				memcpy(p, &pid, sizeof(pid));
				p += sizeof(pid);

				p += payload_len;
				break;
			}
		default:
			not_implemented();
			break;
	}

	return 0;
}
