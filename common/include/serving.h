#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>

struct node {
	pid_t pid;
	int32_t write_fd;
	uint8_t addr;
	uint16_t port;
};

struct serving_data {
	int32_t server_fd;
	struct pollfd* pfds;
	uint32_t pfd_count;
	size_t pfd_capacity;
	socklen_t addrlen;
	struct sockaddr_storage remoteaddr;
	bool (*handle_request)(int32_t sender_fd, void* data);
};

__attribute__((nonnull(1, 3)))
void serving_init(struct serving_data* serving, int32_t server_fd, bool (*handle_request)(int32_t sender_fd, void* data));

void serving_free(struct serving_data* serving);

__attribute__((nonnull(1, 2)))
void serving_poll(struct serving_data* serving, void* data);
