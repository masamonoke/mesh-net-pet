#include "format_server_node.h"

#include <stdlib.h>                // for malloc
#include <string.h>                // for memcpy, strlen, strcpy
#include <unistd.h>                // for getpid

#include "control_utils.h"         // for not_implemented
#include "custom_logger.h"         // for custom_log_error
#include "format.h"                // for request, format_create_base, forma...
#include "format_client_server.h"  // for send_to_node_ret_payload

static int32_t parse_message(enum request* request, void** payload, const uint8_t* buf, size_t buf_len);

static int32_t create_message(enum request request, const void* payload, uint8_t* message, uint32_t* msg_len);

int32_t format_server_node_parse_message(enum request* req, void** payload, const void* buf, size_t len) {
	return parse_message(req, payload, (uint8_t*) buf, len);
}

int32_t format_server_node_create_message(enum request req, const void* payload, uint8_t* buf, uint32_t* len) {
	return create_message(req, payload, buf, len);
}

static int32_t set_node_update_payload(const uint8_t* buf, void* ret_payload);

static int32_t set_send_payload(const uint8_t* buf, struct send_to_node_ret_payload* payload);

static int32_t set_notify_payload(const uint8_t* buf, struct node_notify_payload* payload);

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
		case REQUEST_UPDATE:
			*request = cmd;
			*payload = malloc(sizeof(struct node_update_ret_payload));
			set_node_update_payload(buf, *payload);
			break;
		case REQUEST_PING:
			// no payload needed
			*request = cmd;
			break;
		case REQUEST_SEND:
			{
				*request = cmd;
				*payload = malloc(sizeof(struct send_to_node_ret_payload));
				set_send_payload(buf, *payload);
			}
			break;
		case REQUEST_NOTIFY:
			{
				*request = cmd;
				*payload = malloc(sizeof(struct node_notify_payload));
				set_notify_payload(buf, *payload);
			}
			break;
		default:
			custom_log_error("Unknown server-node command");
			break;
	}

	return 0;
}

static int32_t create_message(enum request request, const void* payload, uint8_t* message, uint32_t* msg_len) {
	uint32_t alias_len;
	uint32_t payload_len;
	int32_t pid;
	uint8_t* p;
	enum request_sender sender;

	switch (request) {
		case REQUEST_UPDATE:
			{
				if (payload) {
					struct node_update_ret_payload* update_payload;
					update_payload = (struct node_update_ret_payload*) payload;
					alias_len = (uint32_t) strlen(update_payload->alias);
					pid = getpid();
					payload_len = sizeof(update_payload->port) + sizeof(alias_len) + alias_len + sizeof(update_payload->label) + sizeof(pid);
					sender = REQUEST_SENDER_NODE;

					*msg_len = payload_len + sizeof(request) + sizeof(*msg_len) + sizeof(sender);

					p = format_create_base(message, *msg_len, request, sender);

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
				} else {
					custom_log_error("Null payload passed");
				}
				break;
			}
		case REQUEST_PING:
			{
				*msg_len = sizeof(sender) + sizeof(request) + sizeof(*msg_len);
				sender = REQUEST_SENDER_SERVER;
				p = format_create_base(message, *msg_len, request, sender);
			}
			break;
		case REQUEST_SEND:
			{
				struct send_to_node_ret_payload* send_payload;

				send_payload = (struct send_to_node_ret_payload*) payload;
				payload_len = sizeof(send_payload->label_from) + sizeof(send_payload->label_to);
				*msg_len = sizeof(sender) + sizeof(request) + sizeof(*msg_len) + payload_len;
				sender = REQUEST_SENDER_SERVER;
				p = format_create_base(message, *msg_len, request, sender);

				// payload
				memcpy(p, &send_payload->label_from, sizeof(send_payload->label_from));
				p += sizeof(send_payload->label_from);
				memcpy(p, &send_payload->label_to, sizeof(send_payload->label_to));
			}
			break;
		case REQUEST_NOTIFY:
			{
				struct node_notify_payload* notify_payload;

				notify_payload = (struct node_notify_payload*) payload;
				payload_len = sizeof(enum notify_type);
				*msg_len = sizeof(sender) + sizeof(request) + sizeof(*msg_len) + payload_len;
				sender = REQUEST_SENDER_NODE;
				p = format_create_base(message, *msg_len, request, sender);
				//payload
				memcpy(p, &notify_payload->notify_type, sizeof(notify_payload->notify_type));
			}
			break;
		default:
			not_implemented();
			break;
	}

	return 0;
}

static int32_t set_node_update_payload(const uint8_t* buf, void* ret_payload) {
	const uint8_t* p;
	uint32_t len;
	char str[32];
	struct node_update_ret_payload* payload;

	payload = (struct node_update_ret_payload*) ret_payload;

	p = format_skip_base(buf);

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
	p += sizeof(payload->pid);

	return 0;
}

static int32_t set_send_payload(const uint8_t* buf, struct send_to_node_ret_payload* payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	// parse payload
	memcpy(&payload->label_from, p, sizeof(payload->label_from));
	p += sizeof(payload->label_from);
	memcpy(&payload->label_to, p, sizeof(payload->label_to));
	return 0;
}

static int32_t set_notify_payload(const uint8_t* buf, struct node_notify_payload* payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	// parse payload
	memcpy(&payload->notify_type, p, sizeof(payload->notify_type));

	return 0;
}
