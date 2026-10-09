#ifndef FIBER_H
#define FIBER_H

typedef enum {
    FIBER_READY,
    FIBER_RUNNING,
    FIBER_BLOCKED,
    FIBER_ZOMBIE
} fiber_state_t;

typedef enum {
    FIBER_PRIORITY_LOW = 1,
    FIBER_PRIORITY_NORMAL = 2,
    FIBER_PRIORITY_HIGH = 3
} fiber_priority_t;

typedef unsigned int fiber_id_t;

typedef void (*fiber_function_t)(void *arg);

/* Optional callback invoked at scheduler checkpoints. */
typedef void (*fiber_monitor_callback_t)(void);

/*
 * Scheduler policies.
 * Priority with aging is the default policy.
 * Round-robin selects READY fibers in cyclic order.
 */
typedef enum {
    FIBER_SCHEDULER_PRIORITY_AGING = 0,
    FIBER_SCHEDULER_ROUND_ROBIN
} fiber_scheduler_policy_t;

/*
 * Scheduler event types used by the dashboard
 * and scheduler trace system.
 */
typedef enum {
    FIBER_EVENT_DISPATCH,
    FIBER_EVENT_YIELD,
    FIBER_EVENT_BLOCK,
    FIBER_EVENT_RESUME,
    FIBER_EVENT_COMPLETE,
    FIBER_EVENT_PRIORITY_CHANGE,
    FIBER_EVENT_CONTEXT_SWITCH
} fiber_event_type_t;

/* Information recorded for every scheduler event. */
typedef struct {
    unsigned long sequence;
    fiber_event_type_t type;
    fiber_id_t fiber_id;
    fiber_state_t state;
    fiber_priority_t priority;
    unsigned long dispatches;
    unsigned long yields;
    unsigned long switches;
    unsigned int aging;
} fiber_event_t;

/*
 * Read-only snapshot of a fiber's current state.
 * Counters are cumulative for the lifetime of the fiber.
 */
typedef struct {
    fiber_id_t id;
    fiber_state_t state;
    fiber_priority_t priority;
    unsigned long dispatches;
    unsigned long yields;
    unsigned long switches;
    unsigned long aging;
    fiber_id_t waiting_for;
} fiber_snapshot_t;

/* Library lifecycle */
int fiber_library_init(void);
void fiber_library_shutdown(void);

/* Fiber creation and configuration */
fiber_id_t fiber_create(fiber_function_t function, void *arg);

int fiber_set_priority(
    fiber_id_t fiber_id,
    fiber_priority_t priority
);

/*
 * Select the policy used by the scheduler.
 * Returns 0 on success and -1 for an invalid policy
 * or when changing the policy while scheduling.
 */
int fiber_set_scheduler_policy(
    fiber_scheduler_policy_t policy
);

fiber_scheduler_policy_t fiber_get_scheduler_policy(void);

/* Scheduling and synchronization */
void fiber_set_monitor_callback(fiber_monitor_callback_t callback);
void fiber_schedule(void);
void fiber_yield(void);
int fiber_join(fiber_id_t fiber_id);

/* Debugging and statistics */
void fiber_debug_dump(void);
void fiber_stats_dump(void);
int fiber_deadlock_detected(void);

/*
 * Retrieve a read-only snapshot of a fiber.
 * Returns 0 on success, -1 if the ID is invalid
 * or snapshot is NULL.
 */
int fiber_get_snapshot(
    fiber_id_t fiber_id,
    fiber_snapshot_t *snapshot
);

/* Scheduler trace functions */
void fiber_trace_clear(void);
void fiber_trace_dump(void);
unsigned long fiber_trace_count(void);

/*
 * Retrieve an individual scheduler event.
 * Returns 0 when the requested event exists.
 * Returns -1 when the index is invalid.
 */
int fiber_trace_get(
    unsigned long index,
    fiber_event_t *event
);

#endif
