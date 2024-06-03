#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <format.h>

#include "custom_logger.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#define node_log_info(format, ...) custom_log_info("Node [" format "]", ##__VA_ARGS__)
#define node_log_error(format, ...) custom_log_error("Node [" format "]", ##__VA_ARGS__)
#define node_log_warn(format, ...) custom_log_warn("Node [" format "]", ##__VA_ARGS__)
#define node_log_debug(format, ...) custom_log_debug("Node [" format "]", ##__VA_ARGS__)

#ifdef __clang__
#pragma clang diagnostic pop
#endif

bool node_essentials_get_conn_and_send(uint16_t port, uint8_t* buf, msg_len_type buf_len);

__attribute__((warn_unused_result))
bool node_essentials_notify_server(notify_t* notify);

__attribute__((nonnull(1)))
void node_essentials_broadcast_route(node_packet_t* route_payload, bool stop_broadcast);

__attribute__((nonnull(1)))
void node_essentials_broadcast(node_packet_t* broadcast_payload);

__attribute__((nonnull(1)))
void node_essentials_unicast(node_packet_t* broadcast_payload);

void node_essentials_reset_connections(void);

void node_essentials_fill_neighbors_port(uint8_t addr);

void node_essentials_send_unicast_contest(unicast_contest_t* unicast);

void node_essentials_send_unicast_first(unicast_contest_t* unicast, uint8_t addr);
