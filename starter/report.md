# Cache-Friendly Graph Processing Report

## Experimental Setup

I used the provided `graph_bench` program without modifying `benchmark.c` or `main.c`. The code was compiled inside WSL with:

```bash
gcc -O2 -std=gnu11 -Wall -Wextra -pedantic -Iinclude \
  -o graph_bench src/main.c src/benchmark.c src/graph_loader.c \
  src/bfs_pointer.c src/graph_csr.c src/bfs_csr.c
```

For benchmark graphs, I used the provided generator:

```bash
python3 scripts/gen_graph.py --kind er --n 10000 --deg 8 --seed 1 --out data/er_10k_deg8_s1.txt
python3 scripts/gen_graph.py --kind er --n 50000 --deg 8 --seed 2 --out data/er_50k_deg8_s2.txt
```

The generated graphs are directed Erd\H{o}s-R\'enyi style graphs with average out-degree 8. Because the generator creates directed edges independently, the BFS starting from vertex `0` does not necessarily reach every vertex; that is why the visited counts are slightly below `n`.

For runtime, I ran each configuration three times and report the median wall-clock time. For Cachegrind, I kept the graph, source vertex, binary, and repeat count fixed across runs so that only the cache model changed.

One important measurement caveat is that the provided benchmark times the whole path inside `run_benchmark`: graph loading for both versions, and graph-to-CSR conversion for the CSR path. To reduce the distortion from one-time setup, I used larger repeat counts so that traversal dominates the total cost.

## 1. Runtime Comparison

The CSR implementation was faster on both tested graph sizes.

| Graph | Repeat count | Pointer times (ms) | CSR times (ms) | Median pointer (ms) | Median CSR (ms) | Speedup |
| --- | ---: | --- | --- | ---: | ---: | ---: |
| `n=10000, deg=8, seed=1` | 100 | `85.46, 76.14, 81.74` | `50.88, 49.35, 48.76` | `81.74` | `49.35` | `1.66x` |
| `n=50000, deg=8, seed=2` | 20 | `175.71, 182.60, 191.74` | `87.34, 67.97, 75.37` | `182.60` | `75.37` | `2.42x` |

Per BFS, the median times are:

- `10000` vertices: pointer `0.817 ms`, CSR `0.494 ms`
- `50000` vertices: pointer `9.13 ms`, CSR `3.77 ms`

The larger graph shows a bigger speedup. That is the expected trend: as the graph grows, the pointer-based layout causes more scattered memory accesses, while CSR still scans mostly contiguous arrays.

Looking at the numbers per BFS makes this more obvious. On the `10000` vertex graph, pointer BFS takes about `0.817 ms` per traversal while CSR takes about `0.494 ms`. On the `50000` vertex graph, pointer BFS rises to about `9.13 ms`, while CSR rises to about `3.77 ms`. The graph is about `5x` larger, but the pointer traversal becomes more than `11x` slower. This means the pointer version is not only paying a fixed overhead per edge; its cost per useful graph operation gets worse as the working set outgrows cache.

Another observation is that the measured CSR time includes conversion into CSR. Even with that setup cost included, CSR is still faster in the repeated benchmark. For a static graph where many traversals or algorithms run after conversion, the one-time conversion cost would be amortized even more strongly.

## 2. Cache Behavior

To compare cache behavior, I profiled the larger graph (`n=50000`, `deg=8`, `seed=2`) with Cachegrind using:

```bash
valgrind --tool=cachegrind --D1=32768,8,64 \
  ./graph_bench --impl=pointer --graph=data/er_50k_deg8_s2.txt --source=0 --repeat=100

valgrind --tool=cachegrind --D1=32768,8,64 \
  ./graph_bench --impl=csr --graph=data/er_50k_deg8_s2.txt --source=0 --repeat=100
```

The main results were:

| Implementation | D1 misses | LLd misses | D1 miss rate | LLd miss rate |
| --- | ---: | ---: | ---: | ---: |
| Pointer | `68,078,892` | `16,919,568` | `19.5%` | `4.8%` |
| CSR | `51,209,989` | `753,402` | `14.2%` | `0.2%` |

Relative to the pointer version, CSR produced:

- about `24.8%` fewer L1 data-cache misses
- about `95.5%` fewer last-level data-cache misses

This is the strongest evidence in the experiment. The D1 reduction is already meaningful, but the LLd reduction is dramatic. Missing in the last-level cache is much more expensive than missing in L1, so this explains why CSR gains a large runtime advantage even though both implementations perform the same high-level BFS algorithm.

The pointer implementation has about `22.5x` more LLd misses than CSR. The runtime speedup is smaller than `22.5x` because both versions still share some costs: queue operations, distance-array initialization, `dist[u]` checks, and branch decisions. These shared costs limit the maximum possible speedup. Still, the large drop in expensive memory stalls is enough to make CSR much faster overall.

The cache results also show a bandwidth issue. In the pointer representation, each fetched cache line may contain only one useful `Edge` node plus pointer fields and padding. In CSR, each fetched line contains many packed neighbor IDs. CSR therefore moves less useless data through the memory hierarchy.

## 3. Why CSR Performs Better

The difference is not algorithmic complexity. Both versions still perform a standard BFS and visit each reachable edge and vertex once. The difference comes from memory layout.

In the pointer graph, each adjacency list is a linked list of heap-allocated `Edge` nodes. When BFS scans neighbors, it repeatedly dereferences `edge->next`. Those nodes may be spread across unrelated heap locations, so the processor sees poor spatial locality and spends time waiting for cache lines that contain only a small amount of immediately useful data.

In CSR, the neighbors of a vertex are stored in a compact slice of `col_idx`, and `row_ptr` tells BFS where that slice starts and ends. Once BFS reaches a vertex, it scans its neighbors through a simple index range:

```c
for (int i = row_ptr[v]; i < row_ptr[v + 1]; i++) {
    int u = col_idx[i];
}
```

That access pattern is much friendlier to caches and hardware prefetchers. A fetched cache line usually contains several nearby neighbor IDs that will be used immediately afterward. CSR also removes per-edge pointer overhead, which makes the representation denser and improves effective cache utilization.

CSR does not make BFS perfectly sequential. The `dist[u]` access is still irregular because `u` depends on graph structure, and the queue order depends on the BFS frontier. However, CSR removes the most frequent pointer-chasing access from the inner loop: fetching the next edge record. That is why CSR has much better cache behavior even though BFS is still a graph traversal with some irregularity.

## 4. Effect of Cache Parameters

I repeated the Cachegrind run on the same larger graph while varying only the L1 data-cache configuration.

### 4.1 Cache Size

Comparing `16 KB, 8-way, 64 B` with `32 KB, 8-way, 64 B`:

| Implementation | D1 misses at 16 KB | D1 misses at 32 KB | Change |
| --- | ---: | ---: | ---: |
| Pointer | `70,094,031` | `68,078,892` | `-2.9%` |
| CSR | `53,824,143` | `51,209,989` | `-4.9%` |

LLd misses changed negligibly:

- Pointer: `16,919,660` to `16,919,568`
- CSR: `753,402` to `753,402`

Increasing cache size helps both implementations, but the effect is modest. A larger L1 captures some short-term reuse, but it does not fundamentally fix the poor locality of pointer chasing. CSR benefits slightly more because its compact layout makes that extra cache capacity more useful.

This suggests that cache capacity alone is not the central bottleneck. If the main issue were simply that the cache was too small, doubling L1 would have changed the results much more. Instead, the key problem is that the pointer layout keeps asking for unrelated heap locations.

### 4.2 Associativity

Comparing `32 KB, 2-way, 64 B` with `32 KB, 8-way, 64 B`:

| Implementation | D1 misses at 2-way | D1 misses at 8-way | Change |
| --- | ---: | ---: | ---: |
| Pointer | `68,133,897` | `68,078,892` | `-0.08%` |
| CSR | `51,277,734` | `51,209,989` | `-0.13%` |

The effect is very small for both versions. This suggests that conflict misses are not the dominant problem here. The main bottleneck is still locality: the pointer version loses because it jumps around memory, not because it maps too many useful blocks to the same set.

This is a useful negative result because it rules out a competing explanation. The pointer implementation is not mainly losing because of unlucky cache-set conflicts; it is losing because its next useful address is hard to predict and often far away from the previous one.

### 4.3 Cache Line Size

At fixed `32 KB` and `8-way` associativity:

| Implementation | 32 B line | 64 B line | 128 B line |
| --- | ---: | ---: | ---: |
| Pointer D1 misses | `86,235,233` | `68,078,892` | `57,539,453` |
| CSR D1 misses | `56,077,999` | `51,209,989` | `48,780,050` |
| Pointer LLd misses | `16,919,588` | `16,919,568` | `7,449,487` |
| CSR LLd misses | `753,439` | `753,402` | `392,633` |

Increasing line size helps both implementations because more nearby data arrives per miss. However, CSR makes much better structural use of that data because adjacent entries in `col_idx` are usually consumed immediately. Pointer BFS also improves, but it still spends much of its time following `next` pointers to unrelated heap nodes. So even after larger cache lines help both versions, CSR remains far better overall.

The README asks whether CSR benefits more from larger cache lines than pointer graphs. The answer is yes in the architectural sense: CSR is the layout that naturally converts larger lines into useful neighbor data. The pointer version also shows a noticeable drop in misses, but that is partly because it starts from a much worse baseline. Even after that drop, CSR still has substantially fewer misses at every tested line size.

Across all cache parameter experiments, the ranking never changes: CSR has fewer D1 misses and dramatically fewer LLd misses. Larger cache size and larger cache lines help, but they do not erase the effect of data layout. Associativity barely matters, which strengthens the conclusion that spatial locality and cache-line utilization are the dominant factors.

The broader takeaway is that hardware features help most when the software layout exposes useful locality. CSR gives the cache and prefetcher a predictable stream of adjacent values. The pointer graph gives the hardware a sequence of dependent pointer dereferences, which is much harder to accelerate.

## Cachegrind Pitfalls and How to Avoid Errors

The biggest risks in this assignment are not code bugs but measurement mistakes.

1. Do not use Cachegrind's slowdown or `time_ms` under Valgrind as your runtime comparison.
   Cachegrind instruments the program heavily, so wall-clock time under Valgrind is not the same as native runtime. Use normal runs for runtime and Cachegrind only for miss statistics.

2. Be careful that the provided benchmark includes setup work.
   In the CSR case, `convert_to_csr()` happens inside the measured benchmark path. If you run only one BFS, CSR can look artificially worse. Using a high repeat count, such as `--repeat=100`, makes the measurement much fairer.

3. Keep the experiment controlled.
   Use the same graph file, source vertex, repeat count, compiler flags, and binary for both implementations. Otherwise the comparison is not meaningful.

4. Compare misses, not just miss rates.
   Miss rates are useful, but absolute miss counts show the real scale of the memory traffic difference.

5. Change one cache parameter at a time.
   If cache size, associativity, and line size are all changed together, it becomes hard to explain which factor caused the result.

6. Report the limitation clearly.
   Because the benchmark includes loading and, for CSR, conversion cost, the Cachegrind numbers are best interpreted as whole-program memory behavior for the provided driver, not as a perfectly isolated BFS-kernel profile.

## Conclusion

The experiments support the main systems lesson of the assignment: two implementations of the same BFS algorithm can behave very differently because of data layout. CSR is consistently faster, produces fewer data-cache misses, and especially reduces expensive last-level misses. Larger caches help only modestly, associativity has little effect, and larger cache lines help both layouts, with CSR remaining the more cache-friendly design throughout.
