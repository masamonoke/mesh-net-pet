#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/file.h>
#include <stdlib.h>
#include <string.h>

#include "custom_logger.h"
#include "control_utils.h"

#ifdef LOG_FILE

static const char* log_filename = "logs.log";

void log_file_(const char* format, ...) {
	FILE* fp;
	va_list args;

	fp = fopen(log_filename, "a");
	if (fp) {
		if (flock(fileno(fp), LOCK_EX)) {
			fprintf(stderr, "Failed to open lock file %s\n", log_filename);
			exit(1);
		}

		va_start(args, format);
		if (vfprintf(fp, format, args) < 0) {
			fprintf(stderr, "Failed to write log\n");
			exit(1);
		}
		if (fprintf(fp, "\n") < 0) {
			fprintf(stderr, "Failed to write log\n");
			exit(1);
		}
		va_end(args);

		if (flock(fileno(fp), LOCK_UN)) {
			fprintf(stderr, "Failed to unlock file %s\n", log_filename);
			exit(1);
		}

	} else {
		fprintf(stderr, "Failed to open log file %s\n", log_filename);
		exit(1);
	}


	fclose(fp);
}

#endif
