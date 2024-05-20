#pragma once

#include <stdint.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#define die(msg, ...) die_(__FUNCTION__, __LINE__, __FILE__, msg, ##__VA_ARGS__)

#define not_implemented() die_(__FUNCTION__, __LINE__, __FILE__, "NOT IMPLEMENTED", "");

#ifdef __clang__
#pragma clang diagnostic pop
#endif

void die_(const char* func_name, int32_t line, const char* file, const char* msg, ...);
