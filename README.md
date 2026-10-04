# FiberLib — User-Level Thread Library

FiberLib is a lightweight user-level thread (fiber) library implemented entirely in C.

The project demonstrates how cooperative multitasking, manual stack management, context switching, scheduling, synchronization, and deadlock detection can be implemented without relying on kernel-level threads.

## Features

- User-level fibers
- Independent manually allocated stacks
- Cooperative context switching
- Priority-aware Round-Robin scheduling
- Priority aging for starvation prevention
- Fiber states:
  - READY
  - RUNNING
  - BLOCKED
  - ZOMBIE
- Fiber creation and execution
- Voluntary yielding with `fiber_yield()`
- Fiber synchronization with `fiber_join()`
- Circular wait / deadlock detection
- Runtime scheduler statistics
- Internal fiber state debugging
- Maximum fiber limit enforcement
- API error handling
- Regression tests
- Example programs
- Makefile-based build system

## Architecture

FiberLib uses a user-space scheduler.

Each fiber is represented by a Fiber Control Block (FCB) containing:

- Fiber ID
- Current state
- Scheduling priority
- CPU execution context
- Allocated stack
- Stack size
- Entry function
- Function argument
- Context-switch count
- Aging information
- Fiber dependency information

The library uses the POSIX `ucontext` API to save and restore execution contexts.

### Scheduling

The scheduler uses priority-aware Round-Robin scheduling.

Higher-priority fibers are preferred, while fibers that remain READY for multiple scheduler rounds gain effective priority through aging.

This prevents lower-priority fibers from being permanently starved.

### Synchronization

`fiber_join()` allows one fiber to wait for another fiber to finish.

A waiting fiber enters the BLOCKED state and is automatically returned to READY when the target fiber completes.

### Deadlock Detection

FiberLib tracks `waiting_for` relationships between blocked fibers.

If a circular dependency is detected, the scheduler reports a deadlock instead of waiting indefinitely.

## Project Structure

```text
FiberLib/
├── Makefile
├── README.md
├── .gitignore
│
├── include/
│   └── fiber.h
│
├── src/
│   └── fiber.c
│
├── examples/
│   ├── create_demo.c
│   ├── join_demo.c
│   ├── priority_demo.c
│   └── round_robin_demo.c
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

Requirements
Linux / Ubuntu
GCC
GNU Make
POSIX ucontext support
Build

Build the complete project:

make

This compiles the FiberLib object file, all examples, and all tests.

Run Tests

Run the complete regression suite:

make test
Run Examples
Basic Fiber Creation
./examples/create_demo
Round-Robin Scheduling
./examples/round_robin_demo
Priority Scheduling
./examples/priority_demo
Fiber Join
./examples/join_demo
Clean Build Artifacts
make clean
API Overview
Initialize
int fiber_library_init(void);

Initializes the FiberLib runtime.

Create a Fiber
fiber_id_t fiber_create(
    fiber_function_t function,
    void *arg
);

Creates a new user-level fiber.

Set Priority
int fiber_set_priority(
    fiber_id_t fiber_id,
    fiber_priority_t priority
);

Sets a fiber's scheduling priority.

Available priorities:

FIBER_PRIORITY_LOW
FIBER_PRIORITY_NORMAL
FIBER_PRIORITY_HIGH
Start Scheduler
void fiber_schedule(void);

Starts the user-space scheduler.

Yield
void fiber_yield(void);

Voluntarily returns execution control to the scheduler.

Join
int fiber_join(fiber_id_t fiber_id);

Waits for another fiber to complete.

Debug State
void fiber_debug_dump(void);

Displays the current state of active fibers.

Scheduler Statistics
void fiber_stats_dump(void);

Displays scheduler runtime statistics.

Deadlock Status
int fiber_deadlock_detected(void);

Returns whether a deadlock was detected during the most recent scheduler execution.

Shutdown
void fiber_library_shutdown(void);

Releases FiberLib resources and shuts down the runtime.

Testing

FiberLib includes tests covering:

Library initialization
Double initialization
Double shutdown
Fiber creation errors
Invalid joins
Self-join prevention
Joining completed fibers
Priority validation
Maximum fiber capacity
Circular join deadlock detection
Priority starvation prevention
Design Goals

The project focuses on understanding the internal mechanisms behind user-level threading rather than hiding them behind a high-level threading framework.

The implementation demonstrates:

Manual stack allocation
Execution-context management
Cooperative scheduling
Scheduling policies
Fiber lifecycle management
Synchronization
Deadlock detection
Runtime debugging and statistics
Limitations

FiberLib is an educational user-level threading library.

It does not provide:

Preemptive scheduling
Kernel-level parallel execution
Automatic synchronization primitives such as mutexes or semaphores
Production-grade signal-based scheduling
Portability to operating systems without the required context API
License

This project is intended for educational and academic use.
