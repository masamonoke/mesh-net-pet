#include <memory.h>
#include <stdlib.h>
#include <assert.h>

#include "format_node_node.h"
#include "format_client_server.h"
#include "custom_logger.h"
#include "control_utils.h"
#include "settings.h"

static void parse_message(enum request* request, void** payload, const uint8_t* buf);

static void create_message(enum request request, const void* payload, uint8_t* message, uint32_t* msg_len);

void format_node_node_parse_message(enum request* req, void** payload, const void* buf) {
	parse_message(req, payload, (uint8_t*) buf);
}

void format_node_node_create_message(enum request req, const void* payload, uint8_t* buf, uint32_t* len) {
	create_message(req, payload, buf, len);
}

static void set_send_payload(const uint8_t* buf, struct send_to_node_ret_payload* payload);

static void set_route_inverse_payload(const uint8_t* buf, struct node_route_inverse_payload* payload);

static void set_route_direct_payload(const uint8_t* buf, struct node_route_direct_payload* payload);

static void parse_message(enum request* request, void** payload, const uint8_t* buf) {
	const uint8_t* p;
	enum request cmd;

	*request = REQUEST_UNDEFINED;

	p = buf;

	memcpy(&cmd, p, sizeof_enum(cmd));

	switch (cmd) {
		case REQUEST_SEND:
			{
				*request = cmd;
				*payload = malloc(sizeof(struct send_to_node_ret_payload));
				set_send_payload(buf, (struct send_to_node_ret_payload*) *payload);
			}
			break;
		case REQUEST_ROUTE_DIRECT:
			{
				*request = cmd;
				*payload = malloc(sizeof(struct node_route_direct_payload));
				set_route_direct_payload(buf, (struct node_route_direct_payload*) *payload);
			}
			break;
		case REQUEST_ROUTE_INVERSE:
			{
				*request = cmd;
				*payload = malloc(sizeof(struct node_route_inverse_payload));
				set_route_inverse_payload(buf, (struct node_route_inverse_payload*) *payload);
			}
			break;
		default:
			custom_log_error("Unknown node-node request");
			break;
	}
}

static void create_message(enum request request, const void* payload, uint8_t* message, uint32_t* msg_len) {
	uint32_t payload_len;
	uint8_t* p;
	enum request_sender sender;

	sender = REQUEST_SENDER_NODE;
	switch (request) {
		case REQUEST_SEND:
			{
				struct send_to_node_ret_payload* ret_payload;

				ret_payload = (struct send_to_node_ret_payload*) payload;

				payload_len = sizeof(ret_payload->addr_to) + sizeof(ret_payload->addr_from) + format_app_message_len(&ret_payload->app_payload);
				*msg_len = payload_len + sizeof_enum(request) + sizeof(*msg_len) + sizeof_enum(sender);
				p = format_create_base(message, *msg_len, request, sender);

				memcpy(p, &ret_payload->addr_from, sizeof(ret_payload->addr_from));
				p += sizeof(ret_payload->addr_from);
				memcpy(p, &ret_payload->addr_to, sizeof(ret_payload->addr_to));
				p += sizeof(ret_payload->addr_to);

				format_app_create_message(&ret_payload->app_payload, p);
			}
			break;
		case REQUEST_ROUTE_DIRECT:
			{
				struct node_route_direct_payload* route_payload;

				route_payload = (struct node_route_direct_payload*) payload;

				payload_len = sizeof(route_payload->local_sender_addr) + sizeof(route_payload->sender_addr) + sizeof(route_payload->receiver_addr) +
					sizeof(route_payload->metric) + sizeof(route_payload->time_to_live) + sizeof(route_payload->id) + format_app_message_len(&route_payload->app_payload);
				*msg_len = payload_len + sizeof_enum(request) + sizeof(*msg_len) + sizeof_enum(sender);

				p = format_create_base(message, *msg_len, request, sender);

				memcpy(p, &route_payload->sender_addr, sizeof(route_payload->sender_addr));
				p += sizeof(route_payload->sender_addr);
				memcpy(p, &route_payload->receiver_addr, sizeof(route_payload->receiver_addr));
				p += sizeof(route_payload->receiver_addr);
				memcpy(p, &route_payload->local_sender_addr, sizeof(route_payload->local_sender_addr));
				p += sizeof(route_payload->local_sender_addr);
				memcpy(p, &route_payload->metric, sizeof(route_payload->metric));
				p += sizeof(route_payload->metric);
				memcpy(p, &route_payload->time_to_live, sizeof(route_payload->time_to_live));
				p += sizeof(route_payload->time_to_live);
				memcpy(p, &route_payload->id, sizeof(route_payload->id));
				p += sizeof(route_payload->id);

				format_app_create_message(&route_payload->app_payload, p);
			}
			break;
		case REQUEST_ROUTE_INVERSE:
			{
				struct node_route_inverse_payload* route_payload;

				route_payload = (struct node_route_inverse_payload*) payload;

				payload_len = sizeof(route_payload->local_sender_addr) + sizeof(route_payload->sender_addr) + sizeof(route_payload->receiver_addr)
					+ sizeof(route_payload->metric) + sizeof(route_payload->time_to_live) + sizeof(route_payload->id);
				*msg_len = payload_len + sizeof_enum(request) + sizeof(*msg_len) + sizeof_enum(sender);

				p = format_create_base(message, *msg_len, request, sender);

				memcpy(p, &route_payload->sender_addr, sizeof(route_payload->sender_addr));
				p += sizeof(route_payload->sender_addr);
				memcpy(p, &route_payload->receiver_addr, sizeof(route_payload->receiver_addr));
				p += sizeof(route_payload->receiver_addr);
				memcpy(p, &route_payload->local_sender_addr, sizeof(route_payload->local_sender_addr));
				p += sizeof(route_payload->local_sender_addr);
				memcpy(p, &route_payload->metric, sizeof(route_payload->metric));
				p += sizeof(route_payload->metric);
				memcpy(p, &route_payload->time_to_live, sizeof(route_payload->time_to_live));
				p += sizeof(route_payload->time_to_live);
				memcpy(p, &route_payload->id, sizeof(route_payload->id));
			}
			break;
		default:
			custom_log_error("Not supported node-node command");
			break;
	}
}

static void set_send_payload(const uint8_t* buf, struct send_to_node_ret_payload* payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	memcpy(&payload->addr_from, p, sizeof(payload->addr_from));
	p += sizeof(payload->addr_from);
	memcpy(&payload->addr_to, p, sizeof(payload->addr_to));
	p += sizeof(payload->addr_to);

	format_app_parse_message(&payload->app_payload, p);
}

static void set_route_inverse_payload(const uint8_t* buf, struct node_route_inverse_payload* payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	// parse payload
	memcpy(&payload->sender_addr, p, sizeof(payload->sender_addr));
	p += sizeof(payload->sender_addr);
	memcpy(&payload->receiver_addr, p, sizeof(payload->receiver_addr));
	p += sizeof(payload->receiver_addr);
	memcpy(&payload->local_sender_addr, p, sizeof(payload->local_sender_addr));
	p += sizeof(payload->local_sender_addr);
	memcpy(&payload->metric, p, sizeof(payload->metric));
	p += sizeof(payload->metric);
	memcpy(&payload->time_to_live, p, sizeof(payload->time_to_live));
	p += sizeof(payload->time_to_live);
	memcpy(&payload->id, p, sizeof(payload->id));
}

static void set_route_direct_payload(const uint8_t* buf, struct node_route_direct_payload* payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	// parse payload
	memcpy(&payload->sender_addr, p, sizeof(payload->sender_addr));
	p += sizeof(payload->sender_addr);
	memcpy(&payload->receiver_addr, p, sizeof(payload->receiver_addr));
	p += sizeof(payload->receiver_addr);
	memcpy(&payload->local_sender_addr, p, sizeof(payload->local_sender_addr));
	p += sizeof(payload->local_sender_addr);
	memcpy(&payload->metric, p, sizeof(payload->metric));
	p += sizeof(payload->metric);
	memcpy(&payload->time_to_live, p, sizeof(payload->time_to_live));
	p += sizeof(payload->time_to_live);
	memcpy(&payload->id, p, sizeof(payload->id));
	p += sizeof(payload->id);

	format_app_parse_message(&payload->app_payload, p);
}
