#pragma once

#include <stdbool.h>     // for bool
#include <stddef.h>      // for size_t
#include <stdint.h>      // for int32_t, uint32_t
#include <sys/socket.h>  // for sockaddr_storage, socklen_t
#include <sys/types.h>   // for pid_t

struct node {
	pid_t pid;
	int32_t write_fd;
	int32_t label;
	int32_t port;
	bool initialized;
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
