#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <format_app.h>

#define msg_len_type uint8_t

#define MSG_BASE_LEN (sizeof_enum(request) + sizeof_enum(sender) + sizeof(msg_len_type))

#define sizeof_packet(packet_ptr) (sizeof(*packet_ptr) - sizeof(packet_ptr->app_payload) + format_app_message_len(&packet_ptr->app_payload))

#define packet_crc(packet_ptr) crc16((uint8_t*) (packet_ptr), sizeof_packet((packet_ptr)) - sizeof((packet_ptr)->crc) - sizeof((packet_ptr)->app_payload.crc))

enum __attribute__((packed, aligned(1))) request_result {
	REQUEST_OK,
	REQUEST_ERR,
	REQUEST_UNKNOWN
};

enum __attribute__((packed, aligned(1))) request_sender {
	REQUEST_SENDER_CLIENT,
	REQUEST_SENDER_NODE,
	REQUEST_SENDER_SERVER,
	REQUEST_SENDER_UNDEFINED
};

enum __attribute__((packed, aligned(1))) request {
	REQUEST_SEND,
	REQUEST_UPDATE,
	REQUEST_PING,
	REQUEST_NOTIFY,
	REQUEST_ROUTE_DIRECT,
	REQUEST_ROUTE_INVERSE,
	REQUEST_KILL_NODE,
	REQUEST_REVIVE_NODE,
	REQUEST_RESET,
	REQUEST_BROADCAST,
	REQUEST_UNICAST,
	REQUEST_UNICAST_CONTEST,
	REQUEST_UNICAST_FIRST,
	REQUEST_UNDEFINED
};

typedef struct __attribute__((__packed__)) node_packet {
	uint8_t sender_addr;
	uint8_t receiver_addr;
	uint8_t local_sender_addr; // from which node request retransmitted
	int8_t time_to_live;
	struct app_payload app_payload;
	uint16_t crc;
} node_packet_t;

typedef struct __attribute__((__packed__)) node_update_payload {
	int32_t pid;
	uint16_t port;
	uint8_t addr;
} node_update_t;

enum __attribute__((packed, aligned(1))) notify_type {
	NOTIFY_GOT_MESSAGE,
	NOTIFY_FAIL,
	NOTIFY_UNICAST_HANDLED
};

typedef struct __attribute__((__packed__)) notify {
	enum notify_type type;
	uint16_t app_msg_id;
} notify_t;

typedef struct __attribute__((__packed__)) unicast_contest {
	enum request req; // either REQUEST_UNICAST_FIRST or REQUEST_UNICAST_CONTEST
	uint8_t node_addr;
	struct app_payload app_payload;
} unicast_contest_t;

__attribute__((nonnull(2)))
void format_sprint_result(enum request_result res, char buf[], size_t len);

__attribute__((warn_unused_result))
enum request_sender format_define_sender(const uint8_t* buf);


__attribute__((warn_unused_result))
bool format_is_message_correct(size_t buf_len, msg_len_type msg_len);

void format_parse(enum request* req, void** payload, const void* buf);

void format_create(enum request req, const void* payload, uint8_t* buf, msg_len_type* len, enum request_sender sender);
