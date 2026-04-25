
#include <stdlib.h>
#include "graph.h"

CSRGraph* convert_to_csr(Graph* g) {
    if (!g) {
        return NULL;
    }

    int n = g->num_vertices;
    int m = 0;
    for (int v = 0; v < n; v++) {
        for (Edge* edge = g->vertices[v].head; edge; edge = edge->next) {
            m++;
        }
    }

    CSRGraph* csr = (CSRGraph*)malloc(sizeof(CSRGraph));
    if (!csr) {
        return NULL;
    }

    csr->num_vertices = n;
    csr->num_edges = m;
    csr->row_ptr = (int*)malloc((size_t)(n + 1) * sizeof(int));
    csr->col_idx = m > 0 ? (int*)malloc((size_t)m * sizeof(int)) : NULL;

    if (!csr->row_ptr || (m > 0 && !csr->col_idx)) {
        free(csr->row_ptr);
        free(csr->col_idx);
        free(csr);
        return NULL;
    }

    int pos = 0;
    for (int v = 0; v < n; v++) {
        csr->row_ptr[v] = pos;
        for (Edge* edge = g->vertices[v].head; edge; edge = edge->next) {
            csr->col_idx[pos++] = edge->dst;
        }
    }
    csr->row_ptr[n] = pos;

    return csr;
}

void free_csr(CSRGraph* g) { if (!g) return; free(g->row_ptr); free(g->col_idx); free(g); }
