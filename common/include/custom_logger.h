#pragma once

#include <stdint.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

// WARNING: do not use this methods in cycles where a lot of output generated if LOG_FILE defined

#define custom_log_info(format, ...) log_file_(LOG_TYPE_INFO, __LINE__, __FILE__, format, ##__VA_ARGS__)

#define custom_log_error(format, ...) log_file_(LOG_TYPE_ERROR, __LINE__, __FILE__, format, ##__VA_ARGS__)

#define custom_log_warn(format, ...) log_file_(LOG_TYPE_WARN, __LINE__, __FILE__, format, ##__VA_ARGS__)

#define custom_log_debug(format, ...) log_file_(LOG_TYPE_DEBUG, __LINE__, __FILE__, format, ##__VA_ARGS__)

#ifdef __clang__
#pragma clang diagnostic pop
#endif

enum log_type {
	LOG_TYPE_INFO,
	LOG_TYPE_ERROR,
	LOG_TYPE_WARN,
	LOG_TYPE_DEBUG
};

void log_file_(enum log_type type, int32_t line, const char* file, const char* format, ...);
