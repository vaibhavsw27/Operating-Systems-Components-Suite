
# Operating Systems Components Suite

A collection of operating system components implemented in **C/C++** on **Unix/Linux**, exploring fundamental concepts such as process management, memory management, executable loading, scheduling, concurrency, and synchronization.

---

## Project Overview

This repository consolidates multiple operating systems assignments into a single project. Each module focuses on implementing a core operating system component from scratch, providing hands-on experience with the internal mechanisms of modern operating systems.

---

## Modules

### ELF Loader
- Implemented a custom 32-bit ELF executable loader.
- Parsed ELF headers and program segments.
- Implemented demand paging to load executable pages on demand, reducing memory overhead.

### Unix Shell
- Developed a Unix-like command-line shell.
- Supported command execution, background processes, I/O redirection, and process creation using POSIX system calls.
- Implemented daemon process handling and basic job control.

### Process Scheduler
- Designed and implemented a priority-based CPU scheduler.
- Managed process states and scheduling policies.
- Simulated CPU scheduling across multiple workloads.

### Multithreading
- Built a user-level multithreading library.
- Implemented thread creation, scheduling, synchronization, and lifecycle management.
- Enabled concurrent execution of multiple tasks.

### Memory Management
- Implemented demand paging for executable loading.
- Optimized virtual memory usage by loading pages only when required.

### EGOS-2000 (Bonus Assignment)
- Extended the EGOS-2000 educational operating system by implementing additional kernel functionality.
- Explored operating system internals including process execution, scheduling, and memory management.

---

## Technologies

- C
- C++
- Unix/Linux
- POSIX System Calls
- Makefile
- ELF Binary Format

---

## Operating System Concepts

- Process Management
- CPU Scheduling
- Multithreading
- Concurrency
- Synchronization
- Memory Management
- Demand Paging
- ELF Loading
- Unix System Programming


---

## Learning Outcomes

This project provided practical experience implementing core operating system mechanisms including executable loading, process scheduling, virtual memory management, multithreading, and Unix system programming. It strengthened my understanding of how modern operating systems manage hardware resources while enabling efficient and concurrent program execution.

```
```
