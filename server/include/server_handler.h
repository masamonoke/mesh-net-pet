#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "serving.h"
#include "format_server_node.h"

__attribute__((nonnull(1), warn_unused_result))
bool handle_ping(const struct node* children, int32_t client_fd, const void* payload);

__attribute__((nonnull(1), warn_unused_result))
bool handle_kill(struct node* children, int32_t label, int32_t client_fd);

__attribute__((nonnull(1), warn_unused_result))
bool handle_notify(const struct node* children, int32_t client_fd, enum notify_type notify);

__attribute__((nonnull(1, 2), warn_unused_result))
bool handle_client_send(struct node* children, const void* payload);

__attribute__((nonnull(1), warn_unused_result))
bool handle_reset(struct node* children, int32_t client_fd);

__attribute__((nonnull(1), warn_unused_result))
bool handle_revive(struct node* children, int32_t label, int32_t client_fd);

__attribute__((nonnull(1, 2)))
void handle_update_child(const void* payload, struct node* children);
