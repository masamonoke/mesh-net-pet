#include "format_server_node.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "control_utils.h"
#include "custom_logger.h"
#include "format.h"
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

static void set_notify_payload(const uint8_t* buf, notify_t* payload);

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
			*payload = malloc(sizeof(node_update_t));
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
				*payload = malloc(SEND_LEN);
				format_parse_send(buf, (send_t*) *payload);
			}
			break;
		case REQUEST_NOTIFY:
			{
				*request = cmd;
				*payload = malloc(sizeof(notify_t));
				set_notify_payload(buf, *payload);
			}
			break;
		case REQUEST_BROADCAST:
		case REQUEST_UNICAST:
			*request = cmd;
			*payload = malloc(BROADCAST_LEN);
			format_parse_broadcast(buf, (broadcast_t*) *payload);
			break;
		default:
			custom_log_error("Unknown server-node command");
			break;
	}
}

static void create_message(enum request request, const void* payload, uint8_t* message, msg_len_type* msg_len) {
	int32_t pid;
	uint8_t* p;
	enum request_sender sender;

	p = NULL;
	switch (request) {
		case REQUEST_UPDATE:
			{
				if (payload) {
					node_update_t* update_payload;
					update_payload = (node_update_t*) payload;
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
				sender = REQUEST_SENDER_SERVER;
				format_create_send(p, payload, message, msg_len, sender);
			}
			break;
		case REQUEST_NOTIFY:
			{
				notify_t* notify_payload;

				notify_payload = (notify_t*) payload;

				*msg_len = MSG_LEN + sizeof(notify_t);

				sender = REQUEST_SENDER_NODE;
				p = format_create_base(message, *msg_len, request, sender);
				//payload
				/* memcpy(p, notify_payload, sizeof_enum(notify_payload)); */
				memcpy(p, &notify_payload->type, sizeof(notify_payload->type));
				p += sizeof(notify_payload->type);
				memcpy(p, &notify_payload->app_msg_id, sizeof(notify_payload->app_msg_id));
			}
			break;
		case REQUEST_BROADCAST:
		case REQUEST_UNICAST:
			if (payload) {
				sender = REQUEST_SENDER_SERVER;
				format_create_broadcast(p, payload, message, msg_len, sender, request);
			}
			break;
		default:
			not_implemented();
			break;
	}
}

static void set_node_update_payload(const uint8_t* buf, void* ret_payload) {
	const uint8_t* p;
	node_update_t* payload;

	payload = (node_update_t*) ret_payload;

	p = format_skip_base(buf);

	// parse payload
	memcpy(&payload->port, p, sizeof(payload->port));
	p += sizeof(payload->port);
	memcpy(&payload->addr, p, sizeof(payload->addr));
	p += sizeof(payload->addr);
	memcpy(&payload->pid, p, sizeof(payload->pid));
	p += sizeof(payload->pid);
}

static void set_notify_payload(const uint8_t* buf, notify_t* payload) {
	const uint8_t* p;
	/* enum_ir tmp; */

	p = format_skip_base(buf);

	// parse payload
	/* memcpy(&tmp, p, sizeof_enum(*payload)); */
	/* *payload = (enum notify_type) tmp; */
	memcpy(&payload->type, p, sizeof(payload->type));
	p += sizeof(payload->type);
	memcpy(&payload->app_msg_id, p, sizeof(payload->app_msg_id));
	p += sizeof(payload->app_msg_id);
}
