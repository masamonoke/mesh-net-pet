#pragma once

#include <stdint.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#define die(msg, ...) die_(__FUNCTION__, __LINE__, __FILE__, msg, ##__VA_ARGS__)

#define not_implemented() die_(__FUNCTION__, __LINE__, __FILE__, "NOT IMPLEMENTED", "");

__attribute__((noreturn)) void die_(const char* func_name, int32_t line, const char* file, const char* msg, ...);

#pragma clang diagnostic pop // NOLINT
