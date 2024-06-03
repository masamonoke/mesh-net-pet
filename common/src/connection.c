#include "connection.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "custom_logger.h"

int32_t connection_socket_to_send(uint16_t port) {
	int32_t server_fd;
	int32_t status;
	struct sockaddr_in addr;
	char buffer[INET_ADDRSTRLEN];

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		custom_log_error("Failed to create socket");
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(port);
	addr.sin_addr.s_addr = ntohl(0);

	status = connect(server_fd, (const struct sockaddr*) &addr, sizeof(addr));
	if (status) {
		inet_ntop(AF_INET, &addr.sin_addr, buffer, sizeof(buffer));
		/* custom_log_error("Failed to connect to server socket on %s:%d", buffer, port); */
		return -1;
	}

	return server_fd;
}

int32_t connection_socket_to_listen(uint16_t port) {
	int32_t fd;
	int32_t rv;
	struct sockaddr_in addr;
	int32_t val;

	fd = socket(AF_INET, SOCK_STREAM, 0);

	if (fd < 0) {
		custom_log_error("Failed to create socket");
		return -1;
	}

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(port);
	addr.sin_addr.s_addr = ntohl(0); // 0.0.0.0
	rv = bind(fd, (const struct sockaddr*) &addr, sizeof(addr));
	if (rv < 0) {
		custom_log_error("Failed to bind() on port %d: %d", port, errno);
		return -1;
	}

	rv = listen(fd, SOMAXCONN);
	if (rv < 0) {
		custom_log_error("Failed to listen() on port %d: %d", port, errno);
		return -1;
	}

	return fd;
}
