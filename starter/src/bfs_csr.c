
#include <stdlib.h>
#include "graph.h"

int bfs_csr(CSRGraph* g, int source, int* dist) {
    if (!g || !dist || source < 0 || source >= g->num_vertices) {
        return -1;
    }

    int n = g->num_vertices;
    for (int i = 0; i < n; i++) {
        dist[i] = -1;
    }

    int* queue = (int*)malloc((size_t)n * sizeof(int));
    if (!queue) {
        return -1;
    }

    int front = 0;
    int back = 0;
    int visited = 0;

    dist[source] = 0;
    queue[back++] = source;

    while (front < back) {
        int v = queue[front++];
        visited++;

        for (int i = g->row_ptr[v]; i < g->row_ptr[v + 1]; i++) {
            int u = g->col_idx[i];
            if (dist[u] == -1) {
                dist[u] = dist[v] + 1;
                queue[back++] = u;
            }
        }
    }

    free(queue);
    return visited;
}
