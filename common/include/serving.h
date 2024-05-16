#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/socket.h>
#include <poll.h>

struct serving_data {
	int32_t server_fd;
	struct pollfd* pfds;
	uint32_t pfd_count;
	size_t pfd_capacity;
	socklen_t addrlen;
	struct sockaddr_storage remoteaddr;
	int32_t (*handle_request)(int32_t sender_fd, void* data);
};

void serving_init(struct serving_data* serving, int32_t server_fd, int32_t (*handle_request)(int32_t sender_fd, void* data));

void serving_free(struct serving_data* serving);

void serving_poll(struct serving_data* serving, void* data);
