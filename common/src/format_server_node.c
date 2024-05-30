#include "format_server_node.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "control_utils.h"
#include "custom_logger.h"
#include "format.h"
#include "format_client_server.h"
#include "settings.h"

static void parse_message(enum request* request, void** payload, const uint8_t* buf);

static void create_message(enum request request, const void* payload, uint8_t* message, msg_len_type* msg_len);

void format_server_node_parse_message(enum request* req, void** payload, const void* buf) {
	parse_message(req, payload, (uint8_t*) buf);
}

void format_server_node_create_message(enum request req, const void* payload, uint8_t* buf, msg_len_type* len) {
	create_message(req, payload, buf, len);
}

static void set_node_update_payload(const uint8_t* buf, void* ret_payload);

static void set_send_payload(const uint8_t* buf, struct send_to_node_ret_payload* payload);

static void set_notify_payload(const uint8_t* buf, enum notify_type* payload);

static void parse_message(enum request* request, void** payload, const uint8_t* buf) {
	const uint8_t* p;
	enum request cmd;
	enum_ir tmp;

	*request = REQUEST_UNDEFINED;

	p = buf;

	memcpy(&tmp, p, sizeof_enum(cmd));
	cmd = (enum request) tmp;

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
				*payload = malloc(sizeof(enum notify_type));
				set_notify_payload(buf, *payload);
			}
			break;
		default:
			custom_log_error("Unknown server-node command");
			break;
	}
}

static void create_message(enum request request, const void* payload, uint8_t* message, msg_len_type* msg_len) {
	uint8_t payload_len;
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
					sender = REQUEST_SENDER_NODE;

					*msg_len = UPDATE_LEN + MSG_LEN;

					p = format_create_base(message, *msg_len, request, sender);

					// payload
					memcpy(p, &update_payload->port, sizeof(update_payload->port));
					p += sizeof(update_payload->port);
					memcpy(p, &update_payload->addr, sizeof(update_payload->addr));
					p += sizeof(update_payload->addr);
					memcpy(p, &pid, sizeof(pid));
					p += sizeof(pid);
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

				payload_len = sizeof(send_payload->addr_to) + sizeof(send_payload->addr_from) + format_app_message_len(&send_payload->app_payload);
				*msg_len = sizeof_enum(sender) + sizeof_enum(request) + sizeof(*msg_len) + payload_len;

				sender = REQUEST_SENDER_SERVER;
				p = format_create_base(message, *msg_len, request, sender);

				// payload
				memcpy(p, &send_payload->addr_from, sizeof(send_payload->addr_from));
				p += sizeof(send_payload->addr_from);
				memcpy(p, &send_payload->addr_to, sizeof(send_payload->addr_to));
				p += sizeof(send_payload->addr_to);

				format_app_create_message(&send_payload->app_payload, p);
			}
			break;
		case REQUEST_NOTIFY:
			{
				enum notify_type* notify_payload;

				notify_payload = (enum notify_type*) payload;

				*msg_len = MSG_LEN + sizeof_enum(notify_payload);

				sender = REQUEST_SENDER_NODE;
				p = format_create_base(message, *msg_len, request, sender);
				//payload
				memcpy(p, notify_payload, sizeof_enum(notify_payload));
			}
			break;
		default:
			not_implemented();
			break;
	}
}

static void set_node_update_payload(const uint8_t* buf, void* ret_payload) {
	const uint8_t* p;
	struct node_update_ret_payload* payload;

	payload = (struct node_update_ret_payload*) ret_payload;

	p = format_skip_base(buf);

	// parse payload
	memcpy(&payload->port, p, sizeof(payload->port));
	p += sizeof(payload->port);
	memcpy(&payload->addr, p, sizeof(payload->addr));
	p += sizeof(payload->addr);
	memcpy(&payload->pid, p, sizeof(payload->pid));
	p += sizeof(payload->pid);
}

static void set_send_payload(const uint8_t* buf, struct send_to_node_ret_payload* payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	// parse payload
	memcpy(&payload->addr_from, p, sizeof(payload->addr_from));
	p += sizeof(payload->addr_from);
	memcpy(&payload->addr_to, p, sizeof(payload->addr_to));
	p += sizeof(payload->addr_to);

	format_app_parse_message(&payload->app_payload, p);
}

static void set_notify_payload(const uint8_t* buf, enum notify_type* payload) {
	const uint8_t* p;
	enum_ir tmp;

	p = format_skip_base(buf);

	// parse payload
	memcpy(&tmp, p, sizeof_enum(*payload));
	*payload = (enum notify_type) tmp;
}
