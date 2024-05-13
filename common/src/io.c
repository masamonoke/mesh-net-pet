#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "io.h"
#include "append_string.h"
#include "custom_logger.h"

int32_t io_read_all(int32_t fd, char* buf_mut, size_t n, size_t* bytes_received) {
	ssize_t rv;

	if (bytes_received != NULL) {
		*bytes_received = 0;
	}

	while (n > 0) {
		rv = read(fd, buf_mut, n);
		if (rv <= 0) {
			return (int32_t) rv; // unexpected eof
		}
		n -= (size_t) rv;
		buf_mut += rv;

		if (bytes_received != NULL) {
			*bytes_received += (size_t) rv;
		}
	}

	return 0;
}

int32_t io_write_all(int32_t fd, const char* buf, size_t n) {
	ssize_t rv;

	while (n > 0) {
		rv = write(fd, buf, n);
		if (rv <= 0) {
			return -1;
		}
		n -= (size_t) rv;
		buf += rv;
	}

	return 0;
}

int32_t io_read_file(uint8_t** bytes_to_send, const char* filename) {
	FILE* fp;
	char* line;
	size_t len;
	ssize_t read;
	append_buf_t buf;

	line = NULL;
	len = 0;
	fp = fopen(filename, "r");
	if (fp == NULL) {
		custom_log_error("Failed to open file %s\n", filename);
		return 1;
	}

	append_string_init(&buf);
	while ((read = getline(&line, &len, fp)) != -1) {
		append_string_append(&buf, line, (size_t) read);
	}

	fclose(fp);

	if (line) {
		free(line);
	}

	*bytes_to_send = (uint8_t*) buf.b;

	return 0;
}

int32_t io_read_file_binary(uint8_t** bytes_to_send, const char* filename, uint32_t* len) {
	FILE* fp;
	uint8_t* buf;
	uint32_t file_len;
	int64_t ret;

	fp = fopen(filename, "rb");
	if (fp == NULL) {
		custom_log_error("Failed to open file %s\n", filename);
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	ret = ftell(fp);
	if (ret < 0) {
		custom_log_error("Failed to open file %s", filename);
		return -1;
	} else {
		file_len = (uint32_t) ret;
	}
	rewind(fp);
	buf = malloc(sizeof(char) * file_len);
	fread(buf, file_len, 1, fp);

	fclose(fp);

	*len = file_len;
	*bytes_to_send = buf;

	return 0;
}

int32_t io_write_file_binary(uint8_t** bytes, const char* filename, uint32_t len) {
	FILE* fp;

	fp = fopen(filename, "wb");
	if (fp == NULL) {
		custom_log_error("Failed to open file %s\n", filename);
		return 1;
	}

	custom_log_debug("Writing %d bytes to file %s", len, filename);
	fwrite(bytes, len, 1, fp);

	return 0;
}

