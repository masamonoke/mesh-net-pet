#include <limits.h>
#include <memory.h>

#include "path.h"

static void dijkstra(const int32_t graph[V][V], int32_t src, int32_t dists[], int32_t next_nodes[][V]);

void path_find(const int32_t graph[V][V], int32_t src, int32_t dists[], int32_t next_nodes[][V]) {
	dijkstra(graph, src, dists, next_nodes);
}

static int32_t minDistance(const int32_t dist[], const int32_t visited[]);

static void set_paths(int32_t parent[], int32_t src, int32_t paths[][V]);

static void dijkstra(const int32_t graph[V][V], int32_t src, int32_t dists[], int32_t next_nodes[][V]) {
    int32_t dist[V];
	int32_t visited[V];
	int32_t parent[V];
	int32_t paths[V][V];

    for (int32_t i = 0; i < V; i++) {
        dist[i] = INT_MAX;
        visited[i] = 0;
        parent[i] = -1;
    }

	for (int32_t i = 0; i < V; i++) {
		for (int32_t j = 0; j < V; j++) {
			paths[i][j] = -1;
			next_nodes[i][j] = -1;
		}
	}

    dist[src] = 0;

    for (int32_t count = 0; count < V - 1; count++) {
        int32_t u = minDistance(dist, visited);

        visited[u] = 1;

        for (int32_t v = 0; v < V; v++) {
            if (!visited[v] && graph[u][v] && dist[u] != INT_MAX && dist[u] + graph[u][v] < dist[v]) {
                dist[v] = dist[u] + graph[u][v];
                parent[v] = u;
            }
        }
    }

	if (dists) {
		memcpy(dists, dist, sizeof(dist));
	}

	set_paths(parent, src, paths);
	for (int32_t i = 0; i < V; i++) {
		for (int32_t j = 0, k = 0; j < V; j++) {
			if (paths[i][j] != -1) {
				next_nodes[i][k++] = paths[i][j];
			}
		}
	}
}

static int32_t minDistance(const int32_t dist[], const int32_t visited[]) {
    int32_t min = INT_MAX, min_index;

    for (int32_t v = 0; v < V; v++) {
        if (!visited[v] && dist[v] <= min) {
            min = dist[v];
            min_index = v;
        }
    }

    return min_index;
}

static void set_path(const int32_t parent[], int32_t j, int32_t path[], int32_t idx);

static void set_paths(int32_t parent[], int32_t src, int32_t paths[][V]) {
    for (int32_t i = 0; i < V; i++) {
        if (i != src) {
            set_path(parent, i, paths[i], V - 1);
        }
    }
}

static void set_path(const int32_t parent[], int32_t j, int32_t path[], int32_t idx) {
    int32_t temp_path[V];
    int32_t k = 0;

    while (j != -1) {
        temp_path[k++] = j;
        j = parent[j];
    }

    for (int32_t i = 0; i < k; i++) {
        path[idx - i] = temp_path[i];
    }

    for (int32_t i = idx - k; i >= 0; i--) {
        path[i] = -1;
    }
}
