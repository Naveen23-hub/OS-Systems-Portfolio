# Operating Systems & Systems Programming Portfolio

A collection of high-performance system simulations implemented in **C** and **C++**. This repository demonstrates a deep understanding of the Linux Kernel, Process Management, Memory Virtualization, and Concurrency.

---

##  Project 1: Unix Shell & CPU Scheduler
**Location:** [`/Custom-Shell-Scheduler`](./Custom-Shell-Scheduler)

A custom command-line interpreter coupled with a user-space Round-Robin CPU scheduler.
* **Shell Features:**
  - Supports standard command execution via `fork()` and `execvp()`.
  - Implements **Piping (`|`)** to chain commands (IPC).
  - Maintains a persistent **Command History** feature.
  - Handles signals (`SIGINT`) for graceful termination.
* **Scheduler Features:**
  - Implements a **Round-Robin** scheduling algorithm with configurable time slices.
  - Manages Context Switching using `SIGSTOP` and `SIGCONT` signals.
  - Uses **Shared Memory (`mmap`)** to share process state between the Shell and Scheduler.

**How to Run:**
```bash
cd Custom-Shell-Scheduler
make
./simple_shell 1 1000  # Run with 1 CPU and 1000ms time slice
```

## Project 2: Smart ELF Loader (Demand Paging)
Location: ['/Smart-ELF-Loader'](./Smart-ELF-Loader)

A custom executable loader that loads ELF binaries into memory. Unlike a standard loader, this implementation uses Demand Paging (Lazy Loading).

A custom executable loader that loads ELF binaries into memory. Unlike a standard loader, this implementation uses **Demand Paging** (Lazy Loading).
* **Key Mechanisms:**
  - **Elf Parsing:** Reads ELF headers to identify `PT_LOAD` segments.
  - **Demand Paging:** Instead of loading the whole file, it sets up a `SIGSEGV` signal handler.
  - **Page Fault Handling:** Catches memory access violations, allocates physical memory via `mmap` on-the-fly, and loads the required page from disk.
  - **Statistics:** Reports total Page Faults and Internal Fragmentation (in KB) after execution.

**How to Run:**
```bash
cd Smart-ELF-Loader
make
./loader fib  # Load and run the fibonacci test program
```

## ⚡ Project 3: C++ Multithreading Library
**Location:** [`/Multithreading-Library`](./Multithreading-Library)

A lightweight header-only library (`simple-multithreader.h`) that abstracts `pthread` complexity, allowing users to parallelize loops effortlessly.
* **Features:**
  - **`parallel_for` API:** A high-level abstraction similar to OpenMP for parallelizing 1D and 2D loops.
  - **Lambda Support:** Accepts C++11 Lambda functions for flexible task definition.
  - **Performance:** Optimized work distribution using `pthread_create` and `pthread_join`.
  - **Demo:** Includes high-performance Matrix Multiplication and Vector Addition examples.

**How to Run:**
```bash
cd Multithreading-Library
make
./matrix 4 1024  # Run matrix multiplication with 4 threads on 1024x1024 size
```

##  Technical Skills Demonstrated
- __System Calls:__ `fork`, `execvp`, `waitpid`, `pipe`, `dup2`.
- __Memory Management:__ `mmap`, `munmap`, Demand Paging, Segmentation.
- __Signal Handling:__ `sigaction` for `SIGSEGV` (Segfaults) and `SIGINT`.
- __Concurrency:__ `pthread`, Mutexes, Race Condition handling.
- __Languages:__ C (Systems Logic), C++ (Threading API).
