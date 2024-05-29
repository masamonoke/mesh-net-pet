#include "format_server_node.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "control_utils.h"
#include "custom_logger.h"
#include "format.h"
#include "format_client_server.h"
#include "settings.h"

static int32_t parse_message(enum request* request, void** payload, const uint8_t* buf);

static int32_t create_message(enum request request, const void* payload, uint8_t* message, uint32_t* msg_len);

int32_t format_server_node_parse_message(enum request* req, void** payload, const void* buf, size_t len) {
	(void) len;
	return parse_message(req, payload, (uint8_t*) buf);
}

int32_t format_server_node_create_message(enum request req, const void* payload, uint8_t* buf, uint32_t* len) {
	return create_message(req, payload, buf, len);
}

static int32_t set_node_update_payload(const uint8_t* buf, void* ret_payload);

static int32_t set_send_payload(const uint8_t* buf, struct send_to_node_ret_payload* payload);

static int32_t set_notify_payload(const uint8_t* buf, struct node_notify_payload* payload);

static int32_t parse_message(enum request* request, void** payload, const uint8_t* buf) {
	const uint8_t* p;
	enum request cmd;

	*request = REQUEST_UNDEFINED;

	p = buf;

	memcpy(&cmd, p, sizeof_enum(cmd));

	switch (cmd) {
		case REQUEST_UPDATE:
			*request = cmd;
			*payload = malloc(sizeof(struct node_update_ret_payload));
			set_node_update_payload(buf, *payload);
			break;
		case REQUEST_PING:
		case REQUEST_STOP_BROADCAST:
		case REQUEST_RESET_BROADCAST:
		case REQUEST_RESET:
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
					pid = getpid();
					payload_len = sizeof(update_payload->port) + sizeof(update_payload->label) + sizeof(pid);
					sender = REQUEST_SENDER_NODE;

					*msg_len = payload_len + sizeof_enum(request) + sizeof_enum(sender) + sizeof(*msg_len);

					p = format_create_base(message, *msg_len, request, sender);

					// payload
					memcpy(p, &update_payload->port, sizeof(update_payload->port));
					p += sizeof(update_payload->port);
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
		case REQUEST_STOP_BROADCAST:
		case REQUEST_RESET_BROADCAST:
		case REQUEST_RESET:
			{
				*msg_len = sizeof_enum(sender) + sizeof_enum(request) + sizeof(*msg_len);
				sender = REQUEST_SENDER_SERVER;
				p = format_create_base(message, *msg_len, request, sender);
			}
			break;
		case REQUEST_SEND:
			{
				struct send_to_node_ret_payload* send_payload;

				send_payload = (struct send_to_node_ret_payload*) payload;
				payload_len = sizeof(send_payload->label_to) + sizeof(send_payload->label_from) + format_app_message_len(&send_payload->app_payload);
				*msg_len = sizeof_enum(sender) + sizeof_enum(request) + sizeof(*msg_len) + payload_len;
				sender = REQUEST_SENDER_SERVER;
				p = format_create_base(message, *msg_len, request, sender);

				// payload
				memcpy(p, &send_payload->label_from, sizeof(send_payload->label_from));
				p += sizeof(send_payload->label_from);
				memcpy(p, &send_payload->label_to, sizeof(send_payload->label_to));
				p += sizeof(send_payload->label_to);

				format_app_create_message(&send_payload->app_payload, p);
			}
			break;
		case REQUEST_NOTIFY:
			{
				struct node_notify_payload* notify_payload;

				notify_payload = (struct node_notify_payload*) payload;
				payload_len = sizeof_enum(enum notify_type);
				*msg_len = sizeof_enum(sender) + sizeof_enum(request) + sizeof(*msg_len) + payload_len;
				sender = REQUEST_SENDER_NODE;
				p = format_create_base(message, *msg_len, request, sender);
				//payload
				memcpy(p, &notify_payload->notify_type, sizeof_enum(notify_payload->notify_type));
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
	struct node_update_ret_payload* payload;

	payload = (struct node_update_ret_payload*) ret_payload;

	p = format_skip_base(buf);

	// parse payload
	memcpy(&payload->port, p, sizeof(payload->port));
	p += sizeof(payload->port);
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
	p += sizeof(payload->label_to);

	format_app_parse_message(&payload->app_payload, p);

	return 0;
}

static int32_t set_notify_payload(const uint8_t* buf, struct node_notify_payload* payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	// parse payload
	memcpy(&payload->notify_type, p, sizeof_enum(payload->notify_type));

	return 0;
}
