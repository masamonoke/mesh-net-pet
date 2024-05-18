#pragma once

#include <stdint.h>

#include "settings.h"

#define V NODE_COUNT

__attribute__((nonnull(1, 4)))
void path_find(const int32_t graph[V][V], int32_t src, int32_t dists[], int32_t next_nodes[][V]);
