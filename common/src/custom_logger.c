#include "custom_logger.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <stdarg.h>

#include "control_utils.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

static const char* log_filename = "logs.log";
static bool die_sentenced = false;

static int32_t write_log(enum log_type type, int32_t line, const char* file, const char* buf, FILE* fp);

// it LOG_FILE not defined log_filename used as sync file
void log_file_(enum log_type type, int32_t line, const char* file, const char* format, ...) { // NOLINT
#ifndef SUPRESS_LOG_OUTPUT
	FILE* fp;
	va_list args;
	char buf[1024];

	fp = fopen(log_filename, "a");
	if (fp) {
		if (flock(fileno(fp), LOCK_EX)) {
			fprintf(stderr, "Failed to open lock file %s\n", log_filename);
			exit(1);
		}

		va_start(args, format);

		if (vsnprintf(buf, sizeof(buf), format, args) < 0 || write_log(type, line, file, buf, fp)) {
			fprintf(stderr, "Failed to write buffer\n");
			die_sentenced = true;
		}

		va_end(args);

		if (flock(fileno(fp), LOCK_UN)) {
			fprintf(stderr, "Failed to unlock file %s\n", log_filename);
			exit(1);
		}

		if (die_sentenced) {
			die("Log writing error");
		}

	} else {
		fprintf(stderr, "Failed to open log file %s\n", log_filename);
		exit(1);
	}


	fclose(fp);
#else
	(void) type;
	(void) line;
	(void) file;
	(void) format;
#endif
}

static int32_t log(const char* color, const char* log_type, int32_t line, const char* file, const char* buf, FILE* fp) {
	(void) fp;

	if (fprintf(stderr, "%s %-6s %-20s %-5d" ANSI_COLOR_RESET ": %-30s\n", color, log_type,  file, line, buf) < 0) {
		return -1;
	}
#ifdef LOG_FILE
	if (fprintf(fp, "%-6s %-20s %-5d: %-30s\n", log_type, file, line, buf) < 0) {
		return - 1;
	}
#endif
	return 0;
}

static int32_t write_log(enum log_type type, int32_t line, const char* file, const char* buf, FILE* fp) { // NOLINT
	int32_t res;

	res = 0;
	switch(type) {
		case LOG_TYPE_INFO:
			res = log(ANSI_COLOR_CYAN, "info", line, file, buf, fp);
			break;
		case LOG_TYPE_ERROR:
			res = log(ANSI_COLOR_RED, "error", line, file, buf, fp);
			break;
		case LOG_TYPE_WARN:
			res = log(ANSI_COLOR_YELLOW, "warn", line, file, buf, fp);
			break;
		case LOG_TYPE_DEBUG:
#ifdef DEBUG
			res = log(ANSI_COLOR_GREEN, "debug", line, file, buf, fp);
#endif
			break;
	}

	return res;
}
