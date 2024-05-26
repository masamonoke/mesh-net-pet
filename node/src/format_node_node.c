#include <memory.h>
#include <stdlib.h>
#include <assert.h>

#include "format_node_node.h"
#include "custom_logger.h"
#include "control_utils.h"
#include "settings.h"

static int32_t parse_message(enum request* request, void** payload, const uint8_t* buf);

static int32_t create_message(enum request request, const void* payload, uint8_t* message, uint32_t* msg_len);

int32_t format_node_node_parse_message(enum request* req, void** payload, const void* buf, size_t len) {
	(void) len;
	return parse_message(req, payload, (uint8_t*) buf);
}

int32_t format_node_node_create_message(enum request req, const void* payload, uint8_t* buf, uint32_t* len) {
	return create_message(req, payload, buf, len);
}

static int32_t set_send_payload(const uint8_t* buf, struct node_send_payload* payload);

static int32_t set_route_payload(const uint8_t* buf, struct node_route_payload* payload);

static int32_t parse_message(enum request* request, void** payload, const uint8_t* buf) {
	const uint8_t* p;
	enum request cmd;

	*request = REQUEST_UNDEFINED;

	p = buf;

	memcpy(&cmd, p, sizeof_enum(cmd));

	switch (cmd) {
		case REQUEST_SEND:
			{
				*request = cmd;
				*payload = malloc(sizeof(struct node_send_payload));
				set_send_payload(buf, (struct node_send_payload*) *payload);
			}
			break;
		case REQUEST_ROUTE_DIRECT:
		case REQUEST_ROUTE_INVERSE:
			{
				*request = cmd;
				*payload = malloc(sizeof(struct node_route_payload));
				set_route_payload(buf, (struct node_route_payload*) *payload);
			}
			break;
		default:
			custom_log_error("Unknown node-node request");
			break;
	}

	return 0;
}

static int32_t create_message(enum request request, const void* payload, uint8_t* message, uint32_t* msg_len) {
	uint32_t payload_len;
	uint8_t* p;
	enum request_sender sender;

	sender = REQUEST_SENDER_NODE;
	switch (request) {
		case REQUEST_SEND:
			{
				struct node_send_payload* send_payload;
				send_payload = (struct node_send_payload*) payload;
				payload_len = sizeof(send_payload->label_to);

				*msg_len = payload_len + sizeof_enum(request) + sizeof(*msg_len) + sizeof_enum(sender);

				p = format_create_base(message, *msg_len, request, sender);

				memcpy(p, &send_payload->label_to, sizeof(send_payload->label_to));
			}
			break;
		case REQUEST_ROUTE_DIRECT:
		case REQUEST_ROUTE_INVERSE:
			{
				struct node_route_payload* route_payload;

				route_payload = (struct node_route_payload*) payload;

				payload_len = sizeof(route_payload->local_sender_label) + sizeof(route_payload->sender_label) + sizeof(route_payload->receiver_label)
					+ sizeof(route_payload->metric) + sizeof(route_payload->time_to_live) + sizeof(route_payload->id);
				*msg_len = payload_len + sizeof_enum(request) + sizeof(*msg_len) + sizeof_enum(sender);

				p = format_create_base(message, *msg_len, request, sender);

				memcpy(p, &route_payload->sender_label, sizeof(route_payload->sender_label));
				p += sizeof(route_payload->sender_label);
				memcpy(p, &route_payload->receiver_label, sizeof(route_payload->receiver_label));
				p += sizeof(route_payload->receiver_label);
				memcpy(p, &route_payload->local_sender_label, sizeof(route_payload->local_sender_label));
				p += sizeof(route_payload->local_sender_label);
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

	return 0;
}

static int32_t set_send_payload(const uint8_t* buf, struct node_send_payload* payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	// parse payload
	memcpy(&payload->label_to, p, sizeof(payload->label_to));

	return 0;
}

static int32_t set_route_payload(const uint8_t* buf, struct node_route_payload* payload) {
	const uint8_t* p;

	p = format_skip_base(buf);

	// parse payload
	memcpy(&payload->sender_label, p, sizeof(payload->sender_label));
	p += sizeof(payload->sender_label);
	memcpy(&payload->receiver_label, p, sizeof(payload->receiver_label));
	p += sizeof(payload->receiver_label);
	memcpy(&payload->local_sender_label, p, sizeof(payload->local_sender_label));
	p += sizeof(payload->local_sender_label);
	memcpy(&payload->metric, p, sizeof(payload->metric));
	p += sizeof(payload->metric);
	memcpy(&payload->time_to_live, p, sizeof(payload->time_to_live));
	p += sizeof(payload->time_to_live);
	memcpy(&payload->id, p, sizeof(payload->id));

	return 0;
}
