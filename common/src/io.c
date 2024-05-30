#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

#include "append_string.h"
#include "custom_logger.h"

bool io_read_all(int32_t fd, uint8_t* buf_mut, msg_len_type n, int16_t* bytes_received) {
	int16_t rv;

	if (bytes_received != NULL) {
		*bytes_received = 0;
	}

	while (n > 0) {
		rv = (int16_t) recv(fd, buf_mut, n, 0);
		if (rv <= 0) {
			if (rv == 0) {
				return true;
			} else {
				custom_log_error("EOF");
				return false;
			}
		}
		n -= rv;
		buf_mut += rv;

		if (bytes_received != NULL) {
			*bytes_received += rv;
		}
	}

	return true;
}

bool io_write_all(int32_t fd, const uint8_t* buf, msg_len_type n) {
	int16_t rv;

	while (n > 0) {
		rv = (int16_t) send(fd, buf, n, 0);
		if (rv <= 0) {
			return false;
		}
		n -= (uint8_t) rv;
		buf += rv;
	}
	return true;
}
