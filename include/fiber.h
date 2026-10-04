#ifndef FIBER_H
#define FIBER_H

typedef enum {
    FIBER_READY,
    FIBER_RUNNING,
    FIBER_BLOCKED,
    FIBER_ZOMBIE
} fiber_state_t;

/*
 * Fiber scheduling priority.
 *
 * Higher priority fibers receive more
 * scheduling opportunities.
 */
typedef enum {
    FIBER_PRIORITY_LOW = 1,
    FIBER_PRIORITY_NORMAL = 2,
    FIBER_PRIORITY_HIGH = 3
} fiber_priority_t;

typedef unsigned int fiber_id_t;

typedef void (*fiber_function_t)(void *arg);

/*
 * Initialize the FiberLib runtime.
 */
int fiber_library_init(void);

/*
 * Shut down the FiberLib runtime
 * and release allocated resources.
 */
void fiber_library_shutdown(void);

/*
 * Create a new user-level fiber
 * with normal scheduling priority.
 *
 * Returns:
 *   > 0  : newly created fiber ID
 *   0    : creation failed
 */
fiber_id_t fiber_create(fiber_function_t function, void *arg);

/*
 * Set the scheduling priority of a fiber.
 *
 * Returns:
 *   0  : success
 *  -1  : failure
 */
int fiber_set_priority(
    fiber_id_t fiber_id,
    fiber_priority_t priority
);

/*
 * Start the user-space scheduler.
 *
 * The scheduler executes READY fibers
 * using priority-aware Round-Robin scheduling.
 */
void fiber_schedule(void);

/*
 * Voluntarily give CPU control back
 * to the scheduler.
 */
void fiber_yield(void);

/*
 * Wait for a fiber to finish execution.
 *
 * Returns:
 *   0  : successfully joined
 *  -1  : invalid fiber ID or invalid operation
 */
int fiber_join(fiber_id_t fiber_id);

/*
 * Display the current internal state
 * of all active fibers.
 */
void fiber_debug_dump(void);

/*
 * Display runtime scheduler statistics.
 */
void fiber_stats_dump(void);

#endif
