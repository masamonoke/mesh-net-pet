#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <format_app.h>

#define msg_len_type uint8_t

#define MSG_LEN_SIZE sizeof(msg_len_type)

#define MSG_LEN (sizeof_enum(request) + sizeof_enum(sender) + MSG_LEN_SIZE)

enum request_result {
	REQUEST_OK,
	REQUEST_ERR,
	REQUEST_UNKNOWN
};

enum request_sender {
	REQUEST_SENDER_CLIENT,
	REQUEST_SENDER_NODE,
	REQUEST_SENDER_SERVER,
	REQUEST_SENDER_UNDEFINED
};

enum request {
	REQUEST_SEND,
	REQUEST_UPDATE,
	REQUEST_PING,
	REQUEST_NOTIFY,
	REQUEST_ROUTE_DIRECT,
	REQUEST_ROUTE_INVERSE,
	REQUEST_STOP_BROADCAST,
	REQUEST_RESET_BROADCAST,
	REQUEST_KILL_NODE,
	REQUEST_REVIVE_NODE,
	REQUEST_RESET,
	REQUEST_UNDEFINED
};

struct send_to_node_ret_payload {
	uint8_t addr_to;
	uint8_t addr_from;
	struct app_payload app_payload;
};

__attribute__((nonnull(2)))
void format_sprint_result(enum request_result res, char buf[], size_t len);

__attribute__((warn_unused_result))
enum request_sender format_define_sender(const uint8_t* buf);

__attribute__((nonnull(1), warn_unused_result))
uint8_t* format_create_base(uint8_t* message, msg_len_type msg_len, enum request cmd, enum request_sender sender);

__attribute__((nonnull(1), warn_unused_result))
uint8_t* format_skip_base(const uint8_t* message);

__attribute__((warn_unused_result))
bool format_is_message_correct(size_t buf_len, msg_len_type msg_len);
