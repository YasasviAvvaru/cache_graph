
#include <stdlib.h>
#include "graph.h"

int bfs_pointer(Graph* g, int source, int* dist) {
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

        for (Edge* edge = g->vertices[v].head; edge; edge = edge->next) {
            int u = edge->dst;
            if (dist[u] == -1) {
                dist[u] = dist[v] + 1;
                queue[back++] = u;
            }
        }
    }

    free(queue);
    return visited;
}
