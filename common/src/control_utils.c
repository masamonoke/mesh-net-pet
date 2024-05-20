#include "control_utils.h"

#include <signal.h>           // for raise
#include <stdint.h>           // for int32_t
#include <stdio.h>            // for fprintf, stderr, vfprintf, va_list
#include <sys/errno.h>        // for errno
#include <sys/signal.h>       // for SIGTERM
#include <stdarg.h>           // for va_start, va_end

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
