# Mini Project 3

Mini-project 3 from CS3.301 Operating Systems and Networks, Monsoon 2026.
Concurrency, distributed shortest paths and symlinks in xv6.

## Structure

```text
mini-project3/
├── concurrency/
│   ├── airport-troubles/
│   └── birthday-boy/
├── xv6/
│   ├── user/
│   ├── kernel/
│   ├── Makefile
│   └── ...
└── README.md
```

## Modules

### 1. Concurrency
- `airport-troubles/`: Simulation of airport arrivals area (students, immigration officer, baggage handlers) using POSIX threads and synchronization primitives without busy waiting.
- `birthday-boy/`: Parallel Bellman-Ford algorithm to compute single-source shortest paths on undirected campus graphs using multithreading.

### 2. xv6 Operating System
- `xv6/`: Implementation of symbolic links (soft links) in xv6 filesystem, including the `symlink` system call, recursive symlink following in `open()`, `O_NOFOLLOW` flag support, and user programs (`ln -s`, `readlink`).
