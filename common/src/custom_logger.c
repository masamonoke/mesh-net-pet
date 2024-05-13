#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "custom_logger.h"

#ifdef LOG_FILE

static const char* log_filename = "logs.log";

// TODO: probably race conditions can happen
void log_file_(const char* format, ...) {
	FILE* fp;
	va_list args;
	int32_t fd;

	fd = open(log_filename, O_APPEND);
	if (fd >= 0) {
		fp = fopen(log_filename, "a");
	} else {
		fp = fopen(log_filename, "w");
	}
	flock(fileno(fp), LOCK_EX);

	va_start(args, format);
	vfprintf(fp, format, args);
	fprintf(fp, "\n");
	va_end(args);
	fclose(fp);

	flock(fileno(fp), LOCK_UN);
}

#endif
