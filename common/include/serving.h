#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>

struct node {
	pid_t pid;
	int32_t write_fd;
	int8_t label;
	int32_t port;
};

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
