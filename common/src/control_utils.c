#include "control_utils.h"

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <stdarg.h>

void die_(const char* func_name, int32_t line, const char* file, const char* msg, ...) { // NOLINT
	int32_t err;
	va_list args;

	va_start(args, msg);
	err = errno;
	fprintf(stderr, "DIED [%s]: (error code: %d), line %d, file %s", func_name, err, line, file);
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	va_end(args);

	raise(SIGTERM);
}
