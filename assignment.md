
# Assignment: Cache-Friendly Graph Processing

Implement BFS over two graph layouts: a pointer-based adjacency list and a CSR graph.

## Build
Run `make` in `starter/`. This must produce `graph_bench`.

## You must implement
- `src/bfs_pointer.c`
- `src/graph_csr.c`
- `src/bfs_csr.c`

## Graph format
First line: `n`, the number of vertices.

Then `n` lines follow. Line `i` describes the outgoing neighbors of vertex `i`:

`deg v1 v2 ... v_deg`

## CLI
Your executable must support:

`./graph_bench --impl=pointer --graph=tests/test_small.txt --source=0 --repeat=10`

and

`./graph_bench --impl=csr --graph=tests/test_small.txt --source=0 --repeat=10`

It must print exactly:

`visited=<count>`
`time_ms=<milliseconds>`

## Visible tests
The tests in `starter/tests/` are only visible sanity checks. The instructor autograder also uses hidden tests.

## Notes
Students only need to build `graph_bench`.
The instructor autograder may temporarily compile helper executables such as `driver_check`.
