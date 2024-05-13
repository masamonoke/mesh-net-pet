#pragma once

#include <log.h>

#ifndef DEBUG
#undef log_debug
#define log_debug(...)
#endif

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#ifdef LOG_FILE
void log_file_(const char* format, ...);

// WARNING: do not use this methods in cycles where a lot of output generated

// TODO: probably better divide logging to file to separate files (logs for nodes, for server etc)

#define custom_log_info(format, ...) log_file_(format, ##__VA_ARGS__); fprintf(stderr, ANSI_COLOR_CYAN); log_info(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#define custom_log_error(format, ...) log_file_(format, ##__VA_ARGS__); fprintf(stderr, ANSI_COLOR_RED); log_error(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#define custom_log_warn(format, ...) log_file_(format, ##__VA_ARGS__); fprintf(stderr, ANSI_COLOR_YELLOW); log_warn(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#define custom_log_debug(format, ...) log_file_(format, ##__VA_ARGS__); fprintf(stderr, ANSI_COLOR_GREEN); log_debug(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#else

#define custom_log_info(format, ...) fprintf(stderr, ANSI_COLOR_CYAN); log_info(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#define custom_log_error(format, ...) fprintf(stderr, ANSI_COLOR_RED); log_error(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#define custom_log_warn(format, ...) fprintf(stderr, ANSI_COLOR_YELLOW); log_warn(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#define custom_log_debug(format, ...) fprintf(stderr, ANSI_COLOR_GREEN); log_debug(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#endif

#pragma clang diagnostic pop // NOLINT

#else

#ifdef LOG_FILE
void log_file_(const char* format, ...);

#define custom_log_info(format, ...) log_file_(format, ##__VA_ARGS__); fprintf(stderr, ANSI_COLOR_CYAN); log_info(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#define custom_log_error(format, ...) log_file_(format, ##__VA_ARGS__); fprintf(stderr, ANSI_COLOR_RED); log_error(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#define custom_log_warn(format, ...) log_file_(format, ##__VA_ARGS__); fprintf(stderr, ANSI_COLOR_YELLOW); log_warn(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#define custom_log_debug(format, ...) log_file_(format, ##__VA_ARGS__); fprintf(stderr, ANSI_COLOR_GREEN); log_debug(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#else

#define custom_log_info(format, ...) fprintf(stderr, ANSI_COLOR_CYAN); log_info(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#define custom_log_error(format, ...) fprintf(stderr, ANSI_COLOR_RED); log_error(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#define custom_log_warn(format, ...) fprintf(stderr, ANSI_COLOR_YELLOW); log_warn(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#define custom_log_debug(format, ...) fprintf(stderr, ANSI_COLOR_GREEN); log_debug(ANSI_COLOR_RESET format, ##__VA_ARGS__)

#endif

#endif
