
#include <stdlib.h>
#include "graph.h"
CSRGraph* convert_to_csr(Graph* g) { (void)g; return NULL; }
void free_csr(CSRGraph* g) { if (!g) return; free(g->row_ptr); free(g->col_idx); free(g); }
