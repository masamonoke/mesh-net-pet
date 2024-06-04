#include "format.h"

#include "settings.h"
#include "custom_logger.h"
#include "control_utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void format_sprint_result(enum request_result res, char buf[], size_t len) {
	switch (res) {
		case REQUEST_OK:
			snprintf(buf, len, "[OK]: %d", res);
			break;
		case REQUEST_ERR:
			snprintf(buf, len, "[ERR]: %d", res);
			break;
		case REQUEST_UNKNOWN:
			snprintf(buf, len, "[UNKNOWN]: %d", res);
			break;
		default:
			custom_log_error("Unknown result response");
			break;
	}
}

enum request_sender format_define_sender(const uint8_t* buf) {
	const uint8_t* p;
	enum request_sender sender;

	p = buf;
	p += sizeof(enum request); // skip cmd
	memcpy(&sender, p, sizeof(sender));

	return sender;
}

bool format_is_message_correct(size_t buf_len, msg_len_type msg_len) {

	if (buf_len > sizeof(msg_len)) {
		if (buf_len != msg_len) {
			custom_log_error("Incorrect message format: buffer length (%d) != delcared message length (%d)", buf_len, msg_len);
			return false;
		}
	} else {
		custom_log_error("Incorrect message format: buffer length (%d) < size of uint32_t", buf_len);
		return false;
	}

	return true;
}

static uint8_t* create_base(uint8_t* message, msg_len_type msg_len, enum request cmd, enum request_sender sender); // NOLINT

static uint8_t* skip_base(const uint8_t* message);

void format_create(enum request req, const void* payload, uint8_t* buf, msg_len_type* len, enum request_sender sender) {
	uint8_t* p;

	p = NULL;
	switch (req) {
		case REQUEST_PING:
		case REQUEST_KILL_NODE:
		case REQUEST_REVIVE_NODE:
		case REQUEST_RESET:
			if (payload) {
				// this requests carry only uint8_t number
				*len = sizeof(uint8_t) + MSG_BASE_LEN;
				p = create_base(buf, *len, req, sender);

				memcpy(p, payload, sizeof(uint8_t));
			} else {
				*len = sizeof(sender) + sizeof(req) + sizeof(*len);
				p = create_base(buf, *len, req, sender);
			}
			break;
		case REQUEST_UPDATE:
			if (payload) {
				int32_t pid;
				node_update_t* update_payload;
				update_payload = (node_update_t*) payload;
				pid = getpid();

				*len = sizeof(node_update_t) + MSG_BASE_LEN;

				p = create_base(buf, *len, req, sender);

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
		case REQUEST_NOTIFY:
			{
				notify_t* notify_payload;

				notify_payload = (notify_t*) payload;

				*len = MSG_BASE_LEN + sizeof(notify_t);

				p = create_base(buf, *len, req, sender);

				//payload
				memcpy(p, &notify_payload->type, sizeof(notify_payload->type));
				p += sizeof(notify_payload->type);
				memcpy(p, &notify_payload->app_msg_id, sizeof(notify_payload->app_msg_id));
			}
			break;
		case REQUEST_ROUTE_DIRECT:
		case REQUEST_ROUTE_INVERSE:
		case REQUEST_SEND:
		case REQUEST_BROADCAST:
		case REQUEST_UNICAST:
			{
				node_packet_t* route_payload;

				route_payload = (node_packet_t*) payload;

				*len = (uint8_t) (sizeof_packet(route_payload) + MSG_BASE_LEN);

				p = create_base(buf, *len, req, sender);

				memcpy(p, &route_payload->sender_addr, sizeof(route_payload->sender_addr));
				p += sizeof(route_payload->sender_addr);
				memcpy(p, &route_payload->receiver_addr, sizeof(route_payload->receiver_addr));
				p += sizeof(route_payload->receiver_addr);
				memcpy(p, &route_payload->local_sender_addr, sizeof(route_payload->local_sender_addr));
				p += sizeof(route_payload->local_sender_addr);
				memcpy(p, &route_payload->time_to_live, sizeof(route_payload->time_to_live));
				p += sizeof(route_payload->time_to_live);
				format_app_create_message(&route_payload->app_payload, p);
				p += format_app_message_len(&route_payload->app_payload);
				memcpy(p, &route_payload->crc, sizeof(route_payload->crc));
			}
			break;
		case REQUEST_UNICAST_CONTEST:
		case REQUEST_UNICAST_FIRST:
			{
				unicast_contest_t* unicast;

				unicast = (unicast_contest_t*) payload;

				*len = sizeof(unicast_contest_t) + MSG_BASE_LEN;

				p = create_base(buf, *len, req, sender);
				memcpy(p, &unicast->req, sizeof(unicast->req));
				p += sizeof(unicast->req);
				memcpy(p, &unicast->node_addr, sizeof(unicast->node_addr));
				p += sizeof(unicast->node_addr);

				format_app_create_message(&unicast->app_payload, p);
			}
			break;
		default:
			not_implemented();
			break;
	}
}

static void parse_addr_payload(const uint8_t* buf, void* ret_payload);

static void parse_route_payload(const uint8_t* buf, node_packet_t* payload);

static void parse_node_update_payload(const uint8_t* buf, node_update_t* payload);

static void parse_notify_payload(const uint8_t* buf, notify_t* payload);

static void parse_unicast_contest_payload(const uint8_t* buf, unicast_contest_t* payload);

void format_parse(enum request* req, void** payload, const void* buf) {
	const uint8_t* p;
	enum request cmd;

	*req = REQUEST_UNDEFINED;
	p = buf;

	memcpy(&cmd, p, sizeof(cmd));

	*req = cmd;
	switch (cmd) {
		case REQUEST_PING:
		case REQUEST_REVIVE_NODE:
		case REQUEST_KILL_NODE:
			*payload = malloc(sizeof(uint8_t));
			parse_addr_payload(buf, *payload);
			break;
		case REQUEST_RESET:
			break;
		case REQUEST_SEND:
		case REQUEST_ROUTE_DIRECT:
		case REQUEST_ROUTE_INVERSE:
		case REQUEST_BROADCAST:
		case REQUEST_UNICAST:
			*payload = malloc(sizeof(node_packet_t));
			parse_route_payload(buf, (node_packet_t*) *payload);
			break;
		case REQUEST_UPDATE:
			*payload = malloc(sizeof(node_update_t));
			parse_node_update_payload(buf, *payload);
			break;
		case REQUEST_NOTIFY:
			*payload = malloc(sizeof(notify_t));
			parse_notify_payload(buf, *payload);
			break;
		case REQUEST_UNICAST_CONTEST:
		case REQUEST_UNICAST_FIRST:
			*payload = malloc(sizeof(unicast_contest_t));
			parse_unicast_contest_payload(buf, *payload);
			break;
		case REQUEST_UNDEFINED:
			custom_log_error("Unknown client-server request");
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

	p = skip_base(buf);

	memcpy(payload, p, sizeof(*payload));
	p += sizeof(payload);
}

static void parse_route_payload(const uint8_t* buf, node_packet_t* payload) {
	const uint8_t* p;

	p = skip_base(buf);

	// parse payload
	memcpy(&payload->sender_addr, p, sizeof(payload->sender_addr));
	p += sizeof(payload->sender_addr);
	memcpy(&payload->receiver_addr, p, sizeof(payload->receiver_addr));
	p += sizeof(payload->receiver_addr);
	memcpy(&payload->local_sender_addr, p, sizeof(payload->local_sender_addr));
	p += sizeof(payload->local_sender_addr);
	memcpy(&payload->time_to_live, p, sizeof(payload->time_to_live));
	p += sizeof(payload->time_to_live);

	format_app_parse_message(&payload->app_payload, p);
	p += format_app_message_len(&payload->app_payload);
	memcpy(&payload->crc, p, sizeof(payload->crc));
}

static void parse_node_update_payload(const uint8_t* buf, node_update_t* payload) {
	const uint8_t* p;

	p = skip_base(buf);

	// parse payload
	memcpy(&payload->port, p, sizeof(payload->port));
	p += sizeof(payload->port);
	memcpy(&payload->addr, p, sizeof(payload->addr));
	p += sizeof(payload->addr);
	memcpy(&payload->pid, p, sizeof(payload->pid));
	p += sizeof(payload->pid);
}

static void parse_unicast_contest_payload(const uint8_t* buf, unicast_contest_t* payload) {
	const uint8_t* p;

	p = skip_base(buf);

	// parse payload
	memcpy(&payload->req, p, sizeof(payload->req));
	p += sizeof(payload->req);
	memcpy(&payload->node_addr, p, sizeof(payload->node_addr));
	p += sizeof(payload->node_addr);
	format_app_parse_message(&payload->app_payload, p);
}

static void parse_notify_payload(const uint8_t* buf, notify_t* payload) {
	const uint8_t* p;

	p = skip_base(buf);

	// parse payload
	memcpy(&payload->type, p, sizeof(payload->type));
	p += sizeof(payload->type);
	memcpy(&payload->app_msg_id, p, sizeof(payload->app_msg_id));
	p += sizeof(payload->app_msg_id);
}

static uint8_t* create_base(uint8_t* message, msg_len_type msg_len, enum request cmd, enum request_sender sender) { // NOLINT
	uint8_t* p;

	p = message;
	memcpy(p, &msg_len, sizeof(msg_len));
	p += sizeof(msg_len);
	memcpy(p, &cmd, sizeof(cmd));
	p += sizeof(cmd);
	memcpy(p, &sender, sizeof(sender));
	p += sizeof(sender);

	return p;
}

static uint8_t* skip_base(const uint8_t* message) {
	uint8_t* p;

	p = (uint8_t*) message;

	p += sizeof(enum request); // skip cmd
	p += sizeof(enum request_sender); // skip sender

	return p;
}
