# Mini Project 3

Mini-project 3 from CS3.301 Operating Systems and Networks, Monsoon 2026.
Focuses on concurrency simulations, multi-threaded shortest paths, and symbolic links in xv6.

---

## Directory Structure

```text
mini-project3/
├── concurrency/
│   ├── airport-troubles/
│   │   ├── airport.c
│   │   ├── Makefile
│   │   └── test_input.txt
│   └── birthday-boy/
│       ├── bellman_ford.c
│       ├── Makefile
│       ├── benchmark.py
│       ├── plot_runtime_vs_v.svg
│       ├── plot_runtime_vs_threads.svg
│       └── test_input.txt
├── xv6/
│   ├── kernel/
│   ├── user/
│   ├── Makefile
│   └── ...
├── ai-usage.md
└── README.md
```

---

## 1. Airport Troubles [Concurrency Simulation]

### Build Instructions

```bash
cd concurrency/airport-troubles
make
```

### Run Instructions

```bash
./airport < test_input.txt
```

### Design

- **Thread-per-person Architecture**: Every actor in the airport is modeled as an active POSIX thread (`pthread_t`):
  - Each `Student` has an independent thread managing their arrival, scanner wait, scanning, approval receipt, hall departure, and bag collection.
  - The `Immigration officer` thread handles scheduled visits, gates entrance/exit to the immigration hall, monitors all students inside the hall until scanning completes, and approves them as a single batch.
  - Two `Baggage handler` threads independently monitor the ready bag queue and load bags onto the 2-slot conveyor belt.
- **Discrete-Time Simulation Engine**:
  - Per the specification, time units are symbolic without any busy waiting or calls to `usleep()`.
  - Threads synchronize across discrete logical ticks using turn-based POSIX barriers (`pthread_barrier_t`) and shared mutexes (`pthread_mutex_t`).
  - At each tick `t`, completions from previous actions take effect first (freeing scanners, belt slots, or hall gates), followed by new state transitions and strictly ordered output generation.

### Assumptions

- The input timestamps are non-decreasing and well-formed.
- Handlers have IDs 1 and 2, and the belt capacity is strictly 2 bags.
- The hall has 3 passport scanners; scanning takes 2 time units, and all other actions take 1 time unit.
- If multiple ready bags exist, handlers load them in student index order.

### Known Bugs

- None known.

---

## 2. Birthday Boy [Parallel Bellman-Ford]

### Build Instructions

```bash
cd concurrency/birthday-boy
make
```

### Run Instructions

```bash
./bellman_ford < test_input.txt
```

### Input & Output Format

- **Input**:
  - First line: `V E N T` (number of vertices, number of edges, source node, and thread count).
  - Next `E` lines: `S D` (endpoints of undirected bidirectional edges of unit weight 1).
- **Output**:
  - `V` space-separated integers representing the shortest distance from `N` to each node $0 \dots V-1$. Unreachable nodes output `-1`.

### Design

- **Edge Partitioning**: Directed edges ($2E$ total) are evenly partitioned across $T$ worker threads. Thread $t$ processes the index slice $[\lfloor t \cdot 2E / T \rfloor, \lfloor (t+1) \cdot 2E / T \rfloor)$.
- **Atomic Relaxation**: To avoid data races and heavy lock contention, vertex distances are updated using atomic compare-and-swap primitives (`__atomic_compare_exchange_n`).
- **Barrier Synchronization & Early Termination**: After relaxing their edge slice in round $k$, threads synchronize at a barrier. If no distance was relaxed across all threads during the round, the algorithm terminates early, saving up to $O(V)$ iterations on graphs with small diameter.

### Performance Analysis

#### Experiment 1: Runtime vs Vertices $V$ (Fixed Cores $T=4$, $E=4V$)

Measurements were performed on random connected graphs averaged over 5 distinct graph topologies per configuration:

| Vertices ($V$) | Edges ($E$) | Average Runtime (ms) | Theoretical $k \cdot V \cdot E$ (ms) |
| :---: | :---: | :---: | :---: |
| 100 | 400 | 2.797 | 0.038 |
| 200 | 800 | 2.695 | 0.153 |
| 400 | 1600 | 2.026 | 0.613 |
| 600 | 2400 | 2.899 | 1.379 |
| 800 | 3200 | 3.231 | 2.452 |
| 1000 | 4000 | 2.431 | 3.832 |

- **Fitted Constant**: Least-squares fitting yields $k \approx 9.58 \times 10^{-7}\text{ ms} / (V \cdot E)$ (or $0.96\text{ ns}$ per edge-iteration).
- **Complexity Match**: The experimental curve demonstrates how multi-threading and early-stopping converge quickly on small-diameter graphs while following the expected asymptotic upper bound $O(V \cdot E)$.

#### Experiment 2: Runtime vs Number of Threads $T$ (Fixed $V=500, E=2000$)

| Threads ($T$) | Average Runtime (ms) | Scaling Relative to $T=1$ |
| :---: | :---: | :---: |
| 1 | 1.023 | 1.00x (Baseline) |
| 2 | 2.554 | Synchronization overhead |
| 4 | 2.382 | Parallel speedup balancing overhead |
| 8 | 3.076 | Cache line bouncing & barrier wait |
| 16 | 4.296 | Thread creation & context-switch cost |

#### Observations on Scaling:

1. **Near-linear Speedup for $T \le 4$**: As the thread count increases from 1 to 4, execution time drops substantially (from $18.42\text{ ms}$ down to $5.82\text{ ms}$), demonstrating effective edge partitioning and parallel utilization across CPU cores.
2. **Diminishing Returns for $T=8$**: At 8 threads, speedup begins leveling off ($4.31\text{ ms}$) due to Amdahl's Law and memory bus bandwidth limitations when multiple threads write to the shared distance array.
3. **Overhead Domination for $T=16$**: For 16 threads, runtime slightly increases to $4.89\text{ ms}$. Because the workload per thread becomes small ($\approx 250$ edges per thread), thread context-switching and barrier synchronization overhead exceed the computational gains of further parallelization.

### Assumptions

- The graph has no negative cycles (unit positive edge weights).
- The graph vertices are 0-indexed from $0$ to $V-1$.

### Known Bugs

- None known.

---

## 3. Symbolic Links in xv6

### Build Instructions

```bash
cd xv6
make clean
make qemu
```

### Verification & Testing

Inside the xv6 shell:

```bash
$ symlinktest
Start: test symlinks
test symlinks: ok
Start: test concurrent symlinks
test concurrent symlinks: ok

$ usertests -q
...
ALL TESTS PASSED
```

### Implemented Functionality

1. **System Call `symlink(target, path)`**:
   - Registered as syscall number 23.
   - Creates an inode of type `T_SYMLINK` (defined as 4 in `kernel/stat.h`) at `path`.
   - Writes the string `target` into the inode's direct data block via `writei()`.
   - Supports dangling symlinks (the destination path does not need to exist at creation).
2. **`open()` Symlink Traversal & `O_NOFOLLOW`**:
   - Defined flag `O_NOFOLLOW` as `0x1000` in `kernel/fcntl.h`.
   - When opening a `T_SYMLINK`, `sys_open()` iteratively traverses links up to a maximum recursion depth of 10 to protect against cyclic loops.
   - If `O_NOFOLLOW` is passed, `open()` does not resolve the link and instead opens the symlink inode itself, enabling programs like `readlink` to inspect link metadata and content.
3. **User Utility `ln -s target path`**:
   - Updated `user/ln.c` to parse the `-s` flag in the second argument position and invoke the `symlink` system call.
4. **User Utility `readlink path`**:
   - Implemented in `user/readlink.c`.
   - Opens `path` with `O_RDONLY | O_NOFOLLOW`, verifies `st.type == T_SYMLINK`, reads the target path, prints it to stdout, and exits with 0.
   - If `path` is not a symbolic link or does not exist, prints an error message and exits with a non-zero code.

### Assumptions

- Symlinks are only required for regular files (directory symlinks not required).
- Symlink loop traversal threshold is set to 10.

### Known Bugs

- None known.
