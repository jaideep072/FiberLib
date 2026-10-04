# FiberLib — User-Level Thread Library

FiberLib is a lightweight **user-level thread (fiber) library written in C**. It implements cooperative scheduling, priority-based scheduling, manual stack management, context switching, synchronization through `fiber_join()`, deadlock detection, priority aging, and runtime statistics completely in user space.

The project demonstrates how a thread scheduler and execution environment can be implemented without relying on kernel-level threads.

---

## Features

* User-level fibers
* Manual fiber stack allocation
* CPU context switching using POSIX `ucontext`
* Cooperative scheduling
* Priority-aware scheduling
* Round-Robin scheduling
* Three scheduling priorities:

  * LOW
  * NORMAL
  * HIGH
* Priority aging to reduce starvation
* Fiber joining and synchronization
* Blocking and unblocking of fibers
* Circular dependency deadlock detection
* Runtime scheduler statistics
* Fiber state inspection
* Dynamic fiber creation through an interactive application
* Maximum fiber limit
* Extensive validation tests

---

## Project Architecture

```text
FiberLib/
├── Makefile
├── README.md
├── main.c
│
├── include/
│   └── fiber.h
│
├── src/
│   └── fiber.c
│
├── examples/
│   ├── create_demo.c
│   ├── round_robin_demo.c
│   ├── priority_demo.c
│   ├── join_demo.c
│   └── fiber_monitor.c
│
└── tests/
    ├── test_init.c
    ├── test_priority.c
    ├── test_invalid_create.c
    ├── test_self_join.c
    ├── test_invalid_join.c
    ├── test_uninitialized.c
    ├── test_double_init.c
    ├── test_double_shutdown.c
    ├── test_join_completed.c
    ├── test_deadlock.c
    ├── test_max_fibers.c
    └── test_starvation.c
```

---

# How to Build

## Requirements

* Ubuntu/Linux
* GCC
* GNU Make
* POSIX `ucontext` support
* Standard C library

Check GCC:

```bash
gcc --version
```

Check Make:

```bash
make --version
```

---

## Build the Complete Project

From the project root:

```bash
make
```

This builds:

* `fiberlib` main application
* FiberLib object file
* all example programs
* all test programs

A clean rebuild can be performed with:

```bash
make clean && make
```

---

# Main Application

The primary way to demonstrate FiberLib is through:

```bash
./fiberlib
```

The application provides an interactive menu:

```text
===============================================
                 FiberLib
          User-Level Thread Library
===============================================
1. Run FiberLib Application
2. Create Custom Fiber
3. Show Library Features
4. Exit
===============================================
```

---

## Option 1 — Run FiberLib Application

Select:

```text
1
```

This launches an application-style demonstration containing:

* Network Monitor
* Report Generator
* Log Collector

The demonstration shows:

* Fiber creation
* Fiber priorities
* Cooperative execution
* Scheduler operation
* Fiber yielding
* `fiber_join()`
* Blocking
* Unblocking
* Fiber completion
* Runtime statistics

The important synchronization sequence is:

```text
[Report Generator] Waiting for network monitor...
[FiberLib] Fiber 3 is waiting for fiber 1.
```

Later:

```text
[FiberLib] Fiber 1 completed execution.
[FiberLib] Fiber 3 unblocked after fiber 1 finished.
```

This demonstrates fiber synchronization using `fiber_join()`.

---

# Option 2 — Create a Custom Fiber

Select:

```text
2
```

The application asks for:

```text
Enter number of work steps (1-10):
```

Then:

```text
Select priority:
1. LOW
2. NORMAL
3. HIGH
```

The application dynamically creates a fiber using:

```c
fiber_create()
```

and applies the selected priority using:

```c
fiber_set_priority()
```

The fiber is then executed by the FiberLib scheduler.

This demonstrates that applications can dynamically create and execute their own user-level fibers.

---

# Option 3 — Show Library Features

Select:

```text
3
```

This displays the major capabilities of FiberLib, including:

* User-level fibers
* Cooperative scheduling
* Priority scheduling
* Round-Robin scheduling
* Priority aging
* Fiber joining
* Deadlock detection
* Runtime statistics
* Dynamic fiber creation

---

# Running Individual Examples

The `examples/` directory contains focused demonstrations of individual FiberLib features.

## 1. Fiber Creation

```bash
./examples/create_demo
```

Demonstrates:

* Library initialization
* Fiber creation
* Fiber state inspection
* Scheduler execution
* Fiber completion
* Library shutdown

---

## 2. Round-Robin Scheduling

```bash
./examples/round_robin_demo
```

Demonstrates cooperative Round-Robin scheduling using multiple fibers.

Typical execution:

```text
[Fiber A] Step 1
[Fiber B] Step 1
[Fiber C] Step 1

[Fiber A] Step 2
[Fiber B] Step 2
[Fiber C] Step 2
```

---

## 3. Priority Scheduling

```bash
./examples/priority_demo
```

Demonstrates:

* LOW priority
* NORMAL priority
* HIGH priority
* Priority-aware scheduling

---

## 4. Fiber Join

```bash
./examples/join_demo
```

Demonstrates one fiber waiting for another fiber to complete using:

```c
fiber_join()
```

---

## 5. FiberMonitor Application Example

```bash
./examples/fiber_monitor
```

This is an application-style example containing:

* Network monitoring
* Log collection
* Report generation

It demonstrates FiberLib in a simulated real-world workload.

---

# Running Tests

All tests can be executed together using:

```bash
make test
```

The Makefile executes every test sequentially.

---

## Run Individual Tests

### Initialization

```bash
./tests/test_init
```

Tests FiberLib initialization.

### Priority

```bash
./tests/test_priority
```

Tests fiber priority assignment.

### Invalid Fiber Creation

```bash
./tests/test_invalid_create
```

Tests invalid fiber creation scenarios.

### Self Join

```bash
./tests/test_self_join
```

Tests prevention of a fiber joining itself.

### Invalid Join

```bash
./tests/test_invalid_join
```

Tests invalid fiber join operations.

### Uninitialized Library

```bash
./tests/test_uninitialized
```

Tests API behavior before library initialization.

### Double Initialization

```bash
./tests/test_double_init
```

Tests repeated library initialization.

### Double Shutdown

```bash
./tests/test_double_shutdown
```

Tests repeated library shutdown.

### Join Completed Fiber

```bash
./tests/test_join_completed
```

Tests joining a fiber that has already completed.

### Deadlock Detection

```bash
./tests/test_deadlock
```

Demonstrates circular fiber dependencies and deadlock detection.

Example dependency:

```text
Fiber A → waits for Fiber B
Fiber B → waits for Fiber A
```

FiberLib detects the circular dependency.

### Maximum Fiber Limit

```bash
./tests/test_max_fibers
```

Tests the maximum number of supported fibers.

Current limit:

```text
128 fibers
```

### Starvation Prevention

```bash
./tests/test_starvation
```

Tests priority aging and verifies that lower-priority fibers still receive CPU time.

---

# Useful Make Commands

## Build Everything

```bash
make
```

## Clean Build Files

```bash
make clean
```

## Rebuild Everything

```bash
make clean && make
```

## Build Examples

```bash
make examples
```

## Build Tests

```bash
make tests
```

## Run All Tests

```bash
make test
```

---

# FiberLib API

The public API is defined in:

```text
include/fiber.h
```

### Initialization

```c
int fiber_library_init(void);
```

Initializes the FiberLib runtime.

### Shutdown

```c
void fiber_library_shutdown(void);
```

Releases FiberLib resources.

### Fiber Creation

```c
fiber_id_t fiber_create(
    fiber_function_t function,
    void *arg
);
```

Creates a new user-level fiber.

### Priority

```c
int fiber_set_priority(
    fiber_id_t fiber_id,
    fiber_priority_t priority
);
```

Sets LOW, NORMAL, or HIGH priority.

### Scheduling

```c
void fiber_schedule(void);
```

Starts the user-level scheduler.

### Yield

```c
void fiber_yield(void);
```

Allows the current fiber to voluntarily return control to the scheduler.

### Join

```c
int fiber_join(fiber_id_t fiber_id);
```

Waits for another fiber to finish.

### Debugging

```c
void fiber_debug_dump(void);
```

Displays the current state of active fibers.

### Statistics

```c
void fiber_stats_dump(void);
```

Displays scheduler statistics.

### Deadlock Detection

```c
int fiber_deadlock_detected(void);
```

Reports whether a deadlock was detected.

---

# Fiber States

Each fiber can exist in one of the following states:

```text
READY
RUNNING
BLOCKED
ZOMBIE
```

### READY

The fiber is ready to execute.

### RUNNING

The fiber is currently executing.

### BLOCKED

The fiber is waiting for another fiber.

### ZOMBIE

The fiber has completed execution and is waiting for cleanup.

---

# Scheduling Design

FiberLib uses **cooperative user-level scheduling**.

A fiber voluntarily gives control back to the scheduler by calling:

```c
fiber_yield();
```

The scheduler then selects another READY fiber.

The scheduler considers:

1. Fiber state
2. Priority
3. Aging
4. Scheduler cursor

This allows FiberLib to demonstrate several operating-system scheduling concepts in user space.

---

# Priority Aging

FiberLib supports three priorities:

```text
LOW       = 1
NORMAL    = 2
HIGH      = 3
```

Aging increases the effective scheduling priority of fibers that have waited for multiple scheduling opportunities.

This helps prevent starvation of lower-priority fibers.

---

# Synchronization

FiberLib provides:

```c
fiber_join()
```

A fiber can wait for another fiber to finish.

Example:

```text
Report Generator
       |
       | waits for
       v
Network Monitor
```

The waiting fiber enters:

```text
BLOCKED
```

When the target fiber completes, the waiting fiber becomes:

```text
READY
```

and continues execution.

---

# Deadlock Detection

FiberLib detects circular dependencies between blocked fibers.

Example:

```text
Fiber A
   |
   | waits for
   v
Fiber B
   |
   | waits for
   v
Fiber A
```

This creates a cycle.

FiberLib detects the cycle and reports a deadlock.

---

# Runtime Statistics

FiberLib tracks:

```text
Total dispatches
Total yield calls
Total context switches
Total completed fibers
Currently active fibers
```

These statistics can be displayed using:

```c
fiber_stats_dump();
```

---

# Testing

The project contains tests covering:

* Initialization
* Shutdown
* Fiber creation
* Priority assignment
* Invalid operations
* Self-join prevention
* Fiber joining
* Deadlock detection
* Maximum fiber capacity
* Starvation prevention
* Scheduler behavior

Run the complete test suite with:

```bash
make test
```

---

# Limitations

FiberLib is an educational user-level threading library.

Current limitations include:

* Cooperative scheduling only
* No preemptive timer-based scheduling
* POSIX `ucontext` dependency
* Fibers execute within a single operating-system process/thread
* No kernel-level parallel execution
* Fixed maximum fiber capacity
* Fixed fiber stack size

The project is intended to demonstrate operating-system concepts rather than replace production threading libraries such as POSIX pthreads.

---

# Design Goals

The main goals of FiberLib are to demonstrate:

* User-level thread management
* Context switching
* Scheduling algorithms
* Synchronization
* Blocking and unblocking
* Deadlock detection
* Starvation prevention
* Resource management
* Operating-system concepts through a practical C implementation

---

# Quick Evaluation Commands

For a quick project demonstration:

### 1. Build

```bash
make clean && make
```

### 2. Run the main application

```bash
./fiberlib
```

### 3. Run Round-Robin demonstration

```bash
./examples/round_robin_demo
```

### 4. Run priority demonstration

```bash
./examples/priority_demo
```

### 5. Run join demonstration

```bash
./examples/join_demo
```

### 6. Demonstrate deadlock detection

```bash
./tests/test_deadlock
```

### 7. Demonstrate starvation prevention

```bash
./tests/test_starvation
```

### 8. Run complete test suite

```bash
make test
```

---

# Cleaning the Project

To remove generated object files and executables:

```bash
make clean
```

After cleaning, rebuild the project using:

```bash
make
```

---

# License

This project is developed as an academic operating-systems project for demonstrating user-level thread and scheduling concepts.
