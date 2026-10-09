# ⚡ FiberLib — A User-Level Thread Library in C

<p align="center">
  <strong>Build your own concurrency. Understand your own scheduler.</strong>
</p>

<p align="center">
  A lightweight user-level fiber library built from scratch in C, featuring context switching, multiple scheduling policies, priority aging, deadlock detection, event tracing, and a live scheduler dashboard.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Language-C-A8B9CC?style=for-the-badge&logo=c&logoColor=white" alt="C Language" />
  <img src="https://img.shields.io/badge/Platform-Linux%20%2F%20Unix-FCC624?style=for-the-badge&logo=linux&logoColor=black" alt="Linux and Unix" />
  <img src="https://img.shields.io/badge/Context-ucontext-blue?style=for-the-badge" alt="ucontext" />
  <img src="https://img.shields.io/badge/Tests-14%20test%20programs-success?style=for-the-badge" alt="14 test programs" />
</p>

---

## 🚀 What is FiberLib?

**FiberLib is an educational user-level threading library that demonstrates how multiple independent execution flows can be managed inside a single process.**

Instead of relying on the operating system to schedule each thread individually, FiberLib creates lightweight *fibers* and manages their execution using its own scheduler.

Each fiber has its own stack, execution context, state, priority, and scheduling statistics. The library switches between fibers using the POSIX `ucontext` mechanism and lets the scheduler decide which READY fiber should execute next.

The result is a small, hands-on model of concurrency and scheduling that makes important operating-system concepts easier to understand through actual code.

### 💡 The central idea

**Don't just use a threading library. Build one and understand what happens underneath.**

---

## ✨ Features at a glance

| Feature                     | What it does                                                |
| --------------------------- | ----------------------------------------------------------- |
| 🧵 User-level fibers        | Runs multiple execution flows inside one process            |
| 💾 Independent stacks       | Gives each fiber its own execution stack                    |
| 🔄 Context switching        | Saves and restores execution contexts using `ucontext`      |
| ⚖️ Round-Robin scheduling   | Selects READY fibers in cyclic order                        |
| 🎯 Priority scheduling      | Favors fibers with higher effective priority                |
| 📈 Priority aging           | Helps prevent low-priority fibers from starving             |
| ⏳ Yielding and joining      | Supports cooperative yielding and waiting for another fiber |
| 🔍 Deadlock detection       | Detects circular wait dependencies                          |
| 📊 Runtime statistics       | Tracks dispatches, yields, switches, and completed fibers   |
| 🧾 Event tracing            | Records scheduler events for inspection                     |
| 🖥️ Live dashboard          | Displays fiber states and scheduler activity                |
| 🧪 Automated tests          | Tests lifecycle handling, scheduling, joins, and edge cases |
| 🛠️ Interactive application | Brings demonstrations and diagnostics together in a menu    |

---

## 🏗️ How FiberLib works

A conventional operating system schedules kernel-managed threads. FiberLib instead manages multiple fibers within one process using a user-space scheduler.

```text
                    FIBERLIB APPLICATION
                            |
                            v
                    +----------------+
                    |  Fiber Library |
                    +----------------+
                            |
                            v
                    +----------------+
                    | User-Space     |
                    | Scheduler      |
                    +----------------+
                            |
              +-------------+-------------+
              |             |             |
              v             v             v
        +-----------+ +-----------+ +-----------+
        | Fiber A   | | Fiber B   | | Fiber C   |
        | Own stack | | Own stack | | Own stack |
        | Context   | | Context   | | Context   |
        +-----------+ +-----------+ +-----------+
              |             |             |
              +-------------+-------------+
                            |
                  Cooperative yielding
                  and context switching
                            |
                            v
                    Scheduler resumes
                    another READY fiber
```

### The lifecycle of a fiber

```text
       Creation
          |
          v
        READY <------------------+
          |                      |
          v                      |
       RUNNING ---- yield -------+
          |
          +------ join/wait ----> BLOCKED
          |                         |
          |                         | Target completes
          |                         v
          |                       READY
          |
          v
        ZOMBIE
```

The scheduler uses four principal states:

* **READY** — the fiber is eligible to execute.
* **RUNNING** — the fiber currently executing.
* **BLOCKED** — the fiber is waiting for another fiber to finish.
* **ZOMBIE** — the fiber's function has completed.

The library uses these states to determine which fibers are eligible for scheduling and which are waiting for completion.

---

## ⚙️ Scheduling systems

One of FiberLib's main features is the ability to compare two scheduling strategies.

### 1. Round-Robin scheduling

Round-Robin gives READY fibers turns in cyclic order.

For three fibers, the execution order can look like this:

```text
Fiber A → Fiber B → Fiber C
    ↑                    |
    +--------------------+
```

**Why it matters:** Round-Robin demonstrates fair cyclic selection without using priority to choose the next fiber.

### 2. Priority + Aging scheduling

FiberLib also supports three priority levels:

* `LOW`
* `NORMAL`
* `HIGH`

The scheduler considers a fiber's effective priority when choosing which READY fiber to run. When effective priorities are equal, cyclic scan order helps determine the selection.

However, strict priority scheduling can cause starvation: a low-priority fiber might wait too long while higher-priority fibers keep getting selected.

FiberLib addresses this with **priority aging**. Waiting READY fibers gradually gain an aging advantage, and their effective priority increases up to the HIGH level.

Example from the starvation test:

| Fiber               | Configured priority | Execution opportunities |
| ------------------- | ------------------- | ----------------------: |
| High-priority fiber | HIGH                |                      15 |
| Low-priority fiber  | LOW                 |                       5 |

The low-priority fiber still receives execution opportunities despite competing with a high-priority fiber.

**Why it matters:** This demonstrates the trade-off between priority-based responsiveness and giving waiting fibers a chance to execute.

---

## 🔄 Context switching: the heart of FiberLib

A fiber must be able to pause its execution and resume later from the same point.

FiberLib uses the POSIX `ucontext` API to manage execution contexts.

Conceptually, a context switch works like this:

1. A fiber begins executing.
2. It calls `fiber_yield()` to voluntarily give up execution.
3. Its current context is saved.
4. Control returns to the scheduler.
5. The scheduler selects another READY fiber.
6. The selected fiber's context is restored.

This is **cooperative scheduling**: fibers normally need to yield or reach another scheduling operation before the scheduler can run a different fiber.

Each fiber has its own stack, with a default size of **64 KiB**.

---

## ⏳ Fiber joining and deadlock detection

FiberLib supports `fiber_join()`, allowing one fiber to wait for another fiber to complete.

For example:

```text
Fiber A ---- waits for ----> Fiber B
                                  |
                                  v
                           Fiber B completes
                                  |
                                  v
                          Fiber A becomes READY
```

The library can also detect circular waiting dependencies.

Consider this situation:

```text
Fiber A waits for Fiber B
       ^              |
       |              v
       +-------- Fiber A
```

Neither fiber can make progress while the circular dependency remains unresolved.

FiberLib checks wait dependencies and reports detected deadlocks, making this operating-system concept observable in a small, reproducible program.

The test suite includes a dedicated circular-join demonstration.

---

## 📊 Scheduler dashboard and event tracing

FiberLib is designed to make scheduling behavior observable, not just executable.

### Live scheduler dashboard

The interactive scheduler dashboard provides a view of:

* Fiber IDs and execution states
* Configured priorities
* Dispatch and yield counts
* Aging information
* Scheduler totals
* Completed fibers
* Event-trace information

The dashboard also offers menu options to inspect the complete event trace and library statistics.

### Event tracing

FiberLib records scheduler events such as:

* `DISPATCH`
* `YIELD`
* `BLOCK`
* `RESUME`
* `COMPLETE`
* `PRIORITY_CHANGE`
* `CONTEXT_SWITCH`

Each event can include a sequence number, fiber identity, state, priority, and relevant counters.

This makes it easier to investigate execution order and understand how the scheduler responds to fiber operations.

### Runtime statistics

The library tracks information including:

* Total dispatches
* Total yield calls
* Total context switches
* Completed fibers
* Active fibers

It also provides fiber snapshots for inspecting an individual fiber's current state and accumulated counters.

**Why this is useful:** The dashboard and trace system turn invisible execution decisions into information you can inspect while learning how scheduling works.

---

## 🧪 Testing and reliability

FiberLib includes 14 test programs covering core behavior and important edge cases.

The test suite covers:

* Library initialization and shutdown
* Invalid priority values
* Invalid fiber creation
* Self-join rejection
* Invalid fiber IDs
* Use before initialization
* Double initialization and shutdown
* Joining completed fibers
* Circular-join deadlock detection
* Maximum fiber capacity
* Starvation prevention
* Round-Robin execution order
* Fiber snapshot behavior

The configured maximum is **128 fibers**.

The complete build and test command was run successfully, with all 14 test programs passing and no compiler warnings or errors reported.

Run the tests yourself to verify the current checkout.

---

## 🛠️ Getting started

### Requirements

* A Linux or compatible Unix-like development environment
* GCC or a compatible C compiler
* GNU Make
* Support for the POSIX `ucontext` API

> **Compatibility note:** `ucontext` is obsolete in POSIX.1-2008 and is not available on every modern platform. This project is intended primarily for educational use in compatible Unix-like environments.

### 1. Clone the repository

```bash
git clone https://github.com/jaideep072/FiberLib.git
cd FiberLib
```

### 2. Build the project

```bash
make
```

### 3. Run the interactive application

```bash
./fiberlib
```

Use the application menu to explore fiber creation, scheduling demonstrations, diagnostics, and the live scheduler dashboard.

### 4. Run the automated tests

```bash
make test
```

### 5. Clean generated build files

```bash
make clean
```

---

## 📁 Project structure

```text
FiberLib/
├── include/
│   └── fiber.h
├── src/
│   └── fiber.c
├── examples/
│   ├── create_demo.c
│   ├── round_robin_demo.c
│   ├── priority_demo.c
│   ├── join_demo.c
│   ├── fiber_monitor.c
│   ├── trace_demo.c
│   └── scheduler_dashboard.c
├── tests/
│   ├── test_init.c
│   ├── test_priority.c
│   ├── test_invalid_create.c
│   ├── test_self_join.c
│   ├── test_invalid_join.c
│   ├── test_uninitialized.c
│   ├── test_double_init.c
│   ├── test_double_shutdown.c
│   ├── test_join_completed.c
│   ├── test_deadlock.c
│   ├── test_max_fibers.c
│   ├── test_starvation.c
│   ├── test_round_robin.c
│   └── test_snapshot.c
├── main.c
├── Makefile
└── README.md
```

* **`include/fiber.h`** — public types and API declarations.
* **`src/fiber.c`** — fiber lifecycle, contexts, scheduler, joining, statistics, and tracing.
* **`examples/`** — demonstrations and interactive monitoring tools.
* **`tests/`** — automated tests for behavior and edge cases.
* **`main.c`** — interactive application entry point.
* **`Makefile`** — build, test, and clean targets.

---

## 🎓 Operating-system concepts demonstrated

FiberLib brings several theoretical concepts into one practical project:

| Concept                     | How the project demonstrates it                     |
| --------------------------- | --------------------------------------------------- |
| Execution contexts          | Saving and restoring fiber contexts                 |
| Stack management            | Allocating a separate stack per fiber               |
| CPU scheduling              | Selecting the next READY fiber                      |
| Scheduling fairness         | Cyclic Round-Robin selection                        |
| Priority scheduling         | Choosing fibers by effective priority               |
| Starvation prevention       | Increasing the effective priority of waiting fibers |
| Synchronization and waiting | Cooperative scheduling and join demonstrations      |
| Deadlocks                   | Detecting circular wait dependencies                |
| Observability               | Runtime statistics, snapshots, and event traces     |

The central learning outcome is understanding how concurrency can be managed in user space and how scheduler design influences execution order.

---

## ⚠️ Design choices and limitations

FiberLib is an educational implementation, not a replacement for production threading libraries.

* **Cooperative execution:** A fiber that never yields can prevent other fibers from running.
* **Single-process scheduling:** The library manages fibers inside one process; fibers are not independently scheduled kernel threads.
* **No automatic preemption:** The scheduler does not forcibly interrupt a running fiber on a timer.
* **Platform dependence:** The implementation relies on `ucontext`, which has limited portability.
* **Single execution thread:** The scheduler is designed for cooperative execution rather than parallel execution across CPU cores.

These limitations are intentional learning opportunities: they help distinguish user-level scheduling from kernel-managed threading and preemptive multitasking.

---

## 🔮 Future enhancements

Potential extensions include:

* Timer-based preemptive scheduling
* Semaphores and condition variables
* More scheduling algorithms
* Improved stack management
* Expanded deadlock and error-path tests
* A graphical scheduler visualizer
* Hybrid user-level and kernel-level scheduling

---

## 💭 Project philosophy

FiberLib is built around a simple idea:

> **Understanding systems begins when you stop treating them as black boxes.**

Instead of only calling an existing threading API, this project explores the mechanics behind fiber creation, execution contexts, scheduling decisions, waiting, and runtime observability.

It is a practical exercise in turning operating-system theory into a working C implementation.

---

## 👨‍💻 Author

**Jaideep**

Explore the source code, run the demonstrations, and inspect the scheduler's behavior:

**[View FiberLib on GitHub →](https://github.com/jaideep072/FiberLib)**

---

<p align="center">
  <strong>FiberLib — Small fibers. Big systems concepts.</strong>
</p>
