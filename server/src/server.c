#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <assert.h>
#include <poll.h>
#include <memory.h>

#include "control_utils.h"
#include "io.h"
#include "custom_logger.h"
#include "format.h"
#include "connection.h"

// TODO: read nodes count, aliases, server port from config or env

#ifndef SERVER_PORT
#define SERVER_PORT 999
#endif

#ifndef NODE_COUNT
#define NODE_COUNT 4
#endif

// to generate names https://frightanic.com/goodies_content/docker-names.php
char* nodes_aliases[NODE_COUNT] = {
	"loving_kare",
	"mad_fermat",
	"boring_almeida",
	"romantic_euclid"
};

// fields alias and is_alive needed for debug only
struct node {
	pid_t pid;
	int32_t fd;
	char* alias;
	int32_t label;
	int32_t port;
	bool initialized;
};

typedef struct server_data {
	int32_t fd;
	struct pollfd* pfds;
	uint32_t fd_count;
	socklen_t addrlen;
	struct sockaddr_storage remoteaddr;
	size_t fd_size;
	struct node children[NODE_COUNT];
} server_data_t;

static volatile bool keeprunning = true;

static void run_node(uint32_t node_label, char* alias, uint32_t port);

static bool is_parent(const struct node* children, size_t len);

static void int_handler(int32_t dummy);

static void server_data_init(server_data_t* server_data);

static void server_data_free(server_data_t* server_data);

static void handle_poll(server_data_t* server_data);

int32_t main(void) {
	size_t i;
	struct server_data server_data;

	signal(SIGINT, int_handler);

	server_data.fd = connection_socket_to_listen();

	if (server_data.fd < 0) {
		die("Failed to get socket");
	}

	for (i = 0; i < NODE_COUNT; i++) {
		server_data.children[i].pid = fork();
		if (server_data.children[i].pid < 0) {
			custom_log_error("Failed to create child process");
			goto L_FREE;
		} else if (server_data.children[i].pid == 0) {
			run_node((uint32_t) i, nodes_aliases[i], (uint32_t) (SERVER_PORT + i + 1));
		} else {
			// parent
			server_data.children[i].initialized = false;
		}
	}

	if (is_parent(server_data.children, NODE_COUNT)) {
		custom_log_info("Started server on port %d (process %d)", SERVER_PORT, getpid());
	}

	server_data_init(&server_data);

	while (keeprunning) {
		// TODO: make non block on accept
		handle_poll(&server_data);
	}

L_FREE:

	server_data_free(&server_data);

	return 0;
}

static void run_node(uint32_t node_label, char* alias, uint32_t port) {
	char node_label_str[16];
	char port_str[16];

	snprintf(node_label_str, sizeof(node_label_str), "%d", node_label);
	snprintf(port_str, sizeof(port_str), "%d", port);

	execl("../node/mesh_node", "mesh_node", "-nodelabel", node_label_str, "-alias", alias, "-port", port_str, (char*) NULL);
}

static bool is_parent(const struct node* children, size_t len) {
	bool is_server;
	size_t i;

	is_server = true;
	for (i = 0; i < len; i++) {
		if (children[i].pid == 0) {
			is_server = false;
			break;
		}
	}

	return is_server;
}

static void int_handler(int32_t dummy) {
	(void) dummy;
	keeprunning = false;
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

static void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static void handle_poll(struct server_data* server_data) {
	size_t i;
	int32_t newfd;
	int32_t poll_count;
	char buf[256];
	char remote_ip[INET6_ADDRSTRLEN];

	poll_count = poll(server_data->pfds, server_data->fd_count, 0);

	if (poll_count == -1) {
		perror("poll");
		exit(1);
	}

	for (i = 0; i < server_data->fd_count; i++) {

		if (server_data->pfds[i].revents & POLLIN) {

			if (server_data->pfds[i].fd == server_data->fd) {
				server_data->addrlen = sizeof(server_data->remoteaddr);
				newfd = accept(server_data->fd, (struct sockaddr *) &server_data->remoteaddr, &server_data->addrlen);

				if (newfd == -1) {
					perror("accept");
				} else {
					add_to_pfds(&server_data->pfds, newfd, &server_data->fd_count, &server_data->fd_size);

					custom_log_info("New connection from %s on socket %d",
						inet_ntop(server_data->remoteaddr.ss_family, get_in_addr((struct sockaddr*) &server_data->remoteaddr), remote_ip, INET6_ADDRSTRLEN), newfd);
				}
			} else {
				int32_t sender_fd;
				ssize_t received_bytes;
				enum request_type cmd_type;
				void* payload;

				received_bytes = recv(server_data->pfds[i].fd, buf, sizeof(buf), 0);

				sender_fd = server_data->pfds[i].fd;

				custom_log_debug("Received %d bytes from client %d", received_bytes, sender_fd);

				if (received_bytes > 0) {
					format_parse_message(&cmd_type, &payload, (uint8_t*) buf, (size_t) received_bytes);
					switch (cmd_type) {
						case REQUEST_TYPE_NODE_UPDATE:
							{
								struct node_upate_ret_payload* ret;
								ret = (struct node_upate_ret_payload*) payload;
								custom_log_debug("Got node update");
								for (i = 0; i < NODE_COUNT; i++) {
									if (server_data->children[i].pid == ret->pid) {
										server_data->children[i].port = ret->port;
										server_data->children[i].label = ret->label;
										server_data->children[i].alias = ret->alias;
										server_data->children[i].initialized = true;
										custom_log_debug("Updated child %d data: label = %d, port = %d, alias = %s",
											server_data->children[i].pid, server_data->children[i].label, server_data->children[i].port, server_data->children[i].alias);
										// TODO: open connection with child
									}
								}
								free(payload);
							}
							break;
						case REQUEST_TYPE_SEND_AS_NODE:
						case REQUEST_TYPE_PING_NODE:
							not_implemented();
							break;
						case REQUEST_TYPE_UNDEFINED:
							custom_log_error("Failed to parse request");
							break;
					}
				} else {
					custom_log_error("Failed to read request from connection %d", sender_fd);
					close(server_data->pfds[i].fd);

					del_from_pfds(server_data->pfds, i, &server_data->fd_count);

					custom_log_info("Socket %d hung up", sender_fd);
				}

			}
		}
	}
}

static void server_data_init(struct server_data* server_data) {
	server_data->fd_count = 0;
	server_data->fd_size = 5;
	server_data->pfds = malloc(sizeof(struct pollfd) * server_data->fd_size);

	server_data->pfds[0].fd = server_data->fd;
    server_data->pfds[0].events = POLLIN;
	server_data->fd_count++;
}

static void server_data_free(struct server_data* server_data) {
	size_t i;

	for (i = 0; i < server_data->fd_count; i++) {
		close(server_data->pfds[i].fd);
	}
	free(server_data->pfds);
}
