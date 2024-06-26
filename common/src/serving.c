#include "serving.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void add_to_pfds(struct pollfd* pfds[], int newfd, uint32_t* fd_count, size_t* fd_size);

static void del_from_pfds(struct pollfd pfds[], size_t i, uint32_t* fd_count);

void serving_poll(struct serving_data* serving, void* data) {
	size_t i;
	int32_t newfd;
	int32_t poll_count;

	poll_count = poll(serving->pfds, serving->pfd_count, 5000);

	if (poll_count == -1) {
		perror("poll");
		exit(1);
	}

	for (i = 0; i < serving->pfd_count; i++) {

		if (serving->pfds[i].revents & POLLIN) {
			if (serving->pfds[i].fd == serving->server_fd) {
				serving->addrlen = sizeof(serving->remoteaddr);
				newfd = accept(serving->server_fd, (struct sockaddr *) &serving->remoteaddr, &serving->addrlen);

				if (newfd == -1) {
					perror("accept");
				} else {
					add_to_pfds(&serving->pfds, newfd, &serving->pfd_count, &serving->pfd_capacity);
				}
			} else {
				int32_t sender_fd;

				sender_fd = serving->pfds[i].fd;
				if (!serving->handle_request(sender_fd, data)) {
					close(sender_fd);
					del_from_pfds(serving->pfds, i, &serving->pfd_count);
				}
			}
		}
	}
}

void serving_init(struct serving_data* serving, int32_t server_fd, bool (*handle_request)(int32_t sender_fd, void* data)) {
	serving->server_fd = server_fd;
	serving->handle_request = handle_request;

	serving->pfd_count = 0;
	serving->pfd_capacity = 5;
	serving->pfds = malloc(sizeof(struct pollfd) * serving->pfd_capacity);

	serving->pfds[0].fd = serving->server_fd;
    serving->pfds[0].events = POLLIN;
	serving->pfd_count++;
}

void serving_free(struct serving_data* serving) {
	size_t i;

	for (i = 0; i < serving->pfd_count; i++) {
		close(serving->pfds[i].fd);
	}
	free(serving->pfds);
}

static void add_to_pfds(struct pollfd* pfds[], int newfd, uint32_t* fd_count, size_t* fd_size) {
    if (*fd_count == *fd_size) {
        *fd_size *= 2;

        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN;

    (*fd_count)++;
}

static void del_from_pfds(struct pollfd pfds[], size_t i, uint32_t* fd_count) {
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}
