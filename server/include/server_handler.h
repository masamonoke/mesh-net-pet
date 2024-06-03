#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "serving.h"
#include "format.h"

__attribute__((nonnull(1), warn_unused_result))
bool handle_ping(const struct node* children, int32_t client_fd, const void* payload);

__attribute__((nonnull(1), warn_unused_result))
bool handle_kill(struct node* children, uint8_t addr, int32_t client_fd);

__attribute__((nonnull(2), warn_unused_result))
bool handle_notify(int32_t client_fd, notify_t* notify);

__attribute__((nonnull(1, 2), warn_unused_result))
bool handle_client_send(struct node* children, const void* payload);

__attribute__((nonnull(1, 2), warn_unused_result))
bool handle_broadcast(struct node* children, const void* payload, enum request cmd);

__attribute__((nonnull(1), warn_unused_result))
bool handle_reset(struct node* children, int32_t client_fd);

__attribute__((nonnull(1), warn_unused_result))
bool handle_revive(struct node* children, uint8_t addr, int32_t client_fd);

__attribute__((nonnull(1, 2)))
void handle_update_child(const void* payload, struct node* children);
