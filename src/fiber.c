#include "../include/fiber.h"
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIBER_STACK_SIZE (64 * 1024)
#define MAX_FIBERS 128

/*
 * Number of scheduler rounds a fiber must wait
 * before receiving one level of effective priority.
 */
#define AGING_THRESHOLD 5

typedef struct fiber_control_block {
    fiber_id_t id;
    fiber_state_t state;
    fiber_priority_t priority;

    ucontext_t context;

    void *stack;
    size_t stack_size;

    fiber_function_t function;
    void *arg;

    unsigned long switches;

    /*
     * Number of scheduler rounds this fiber has
     * remained READY without being selected.
     */
    unsigned long aging;

    fiber_id_t waiting_for;
} fiber_control_block_t;

static fiber_control_block_t fibers[MAX_FIBERS];

static unsigned int fiber_count = 0;
static fiber_id_t next_fiber_id = 1;

static unsigned int scheduler_cursor = 0;

static ucontext_t scheduler_context;

static int library_initialized = 0;
static int scheduler_running = 0;

static int deadlock_detected = 0;

static fiber_control_block_t *current_fiber = NULL;

/*
 * Global scheduler statistics.
 */
static unsigned long total_dispatches = 0;
static unsigned long total_yields = 0;
static unsigned long total_context_switches = 0;
static unsigned long total_completed = 0;

/*
 * Convert a fiber state into a readable string.
 */
static const char *state_to_string(fiber_state_t state)
{
    switch (state) {
        case FIBER_READY:
            return "READY";

        case FIBER_RUNNING:
            return "RUNNING";

        case FIBER_BLOCKED:
            return "BLOCKED";

        case FIBER_ZOMBIE:
            return "ZOMBIE";

        default:
            return "UNKNOWN";
    }
}

/*
 * Convert a priority value into a readable string.
 */
static const char *priority_to_string(fiber_priority_t priority)
{
    switch (priority) {
        case FIBER_PRIORITY_LOW:
            return "LOW";

        case FIBER_PRIORITY_NORMAL:
            return "NORMAL";

        case FIBER_PRIORITY_HIGH:
            return "HIGH";

        default:
            return "UNKNOWN";
    }
}

/*
 * Reset the complete fiber table and scheduler state.
 */
static void initialize_fiber_table(void)
{
    memset(fibers, 0, sizeof(fibers));

    for (int i = 0; i < MAX_FIBERS; i++) {
        fibers[i].state = FIBER_ZOMBIE;
    }

    fiber_count = 0;
    next_fiber_id = 1;
    scheduler_cursor = 0;

    deadlock_detected = 0;

    total_dispatches = 0;
    total_yields = 0;
    total_context_switches = 0;
    total_completed = 0;
}

/*
 * Find an active fiber by ID.
 */
static fiber_control_block_t *find_fiber(fiber_id_t id)
{
    for (unsigned int i = 0; i < MAX_FIBERS; i++) {
        if (fibers[i].id == id &&
            fibers[i].state != FIBER_ZOMBIE) {

            return &fibers[i];
        }
    }

    return NULL;
}

/*
 * Find a fiber regardless of its current state.
 */
static fiber_control_block_t *find_fiber_any_state(fiber_id_t id)
{
    for (unsigned int i = 0; i < MAX_FIBERS; i++) {
        if (fibers[i].id == id) {
            return &fibers[i];
        }
    }

    return NULL;
}

/*
 * Find an unused FCB slot.
 */
static fiber_control_block_t *allocate_fcb(void)
{
    for (unsigned int i = 0; i < MAX_FIBERS; i++) {
        if (fibers[i].state == FIBER_ZOMBIE) {
            return &fibers[i];
        }
    }

    return NULL;
}

/*
 * Release all resources associated with an FCB.
 */
static void release_fcb(fiber_control_block_t *fcb)
{
    if (fcb == NULL) {
        return;
    }

    if (fcb->stack != NULL) {
        free(fcb->stack);
        fcb->stack = NULL;
    }

    fcb->stack_size = 0;
    fcb->function = NULL;
    fcb->arg = NULL;
    fcb->switches = 0;
    fcb->aging = 0;
    fcb->waiting_for = 0;
    fcb->priority = FIBER_PRIORITY_NORMAL;
    fcb->state = FIBER_ZOMBIE;
    fcb->id = 0;
}

/*
 * Calculate the effective scheduling priority.
 *
 * Base priority:
 *   LOW    = 1
 *   NORMAL = 2
 *   HIGH   = 3
 *
 * Aging increases the effective priority of a fiber
 * that has been waiting for multiple scheduler rounds.
 */
static unsigned long effective_priority(
    const fiber_control_block_t *fcb)
{
    return (unsigned long)fcb->priority +
           (fcb->aging / AGING_THRESHOLD);
}

/*
 * Priority-aware Round-Robin scheduler with aging.
 *
 * Higher effective priority is preferred.
 *
 * Aging prevents starvation:
 * a READY fiber that waits long enough gradually
 * receives a higher effective priority.
 *
 * Fibers with the same effective priority are selected
 * using Round-Robin order.
 */
static fiber_control_block_t *find_next_ready(void)
{
    unsigned long highest_effective_priority = 0;

    /*
     * Increase the aging value of every READY fiber
     * before selecting the next fiber.
     */
    for (unsigned int i = 0; i < MAX_FIBERS; i++) {
        if (fibers[i].state == FIBER_READY) {
            fibers[i].aging++;
        }
    }

    /*
     * Find the highest effective priority.
     */
    for (unsigned int i = 0; i < MAX_FIBERS; i++) {
        if (fibers[i].state == FIBER_READY) {
            unsigned long effective =
                effective_priority(&fibers[i]);

            if (effective > highest_effective_priority) {
                highest_effective_priority = effective;
            }
        }
    }

    if (highest_effective_priority == 0) {
        return NULL;
    }

    /*
     * Select a fiber with the highest effective priority,
     * starting from the scheduler cursor.
     */
    for (unsigned int offset = 0;
         offset < MAX_FIBERS;
         offset++) {

        unsigned int index =
            (scheduler_cursor + offset) % MAX_FIBERS;

        if (fibers[index].state == FIBER_READY &&
            effective_priority(&fibers[index]) ==
                highest_effective_priority) {

            scheduler_cursor =
                (index + 1) % MAX_FIBERS;

            /*
             * Reset aging after the fiber receives CPU time.
             */
            fibers[index].aging = 0;

            return &fibers[index];
        }
    }

    return NULL;
}

/*
 * Wake fibers waiting for a completed fiber.
 */
static void wake_waiting_fibers(fiber_id_t completed_id)
{
    for (unsigned int i = 0; i < MAX_FIBERS; i++) {
        if (fibers[i].state == FIBER_BLOCKED &&
            fibers[i].waiting_for == completed_id) {

            fibers[i].state = FIBER_READY;
            fibers[i].waiting_for = 0;
            fibers[i].aging = 0;

            printf(
                "[FiberLib] Fiber %u unblocked after fiber %u finished.\n",
                fibers[i].id,
                completed_id
            );
        }
    }
}

/*
 * Detect whether a blocked fiber is part of a circular
 * wait-for dependency.
 */
static int has_wait_cycle(
    fiber_id_t start_id)
{
    fiber_id_t current_id = start_id;

    for (unsigned int depth = 0;
         depth < MAX_FIBERS;
         depth++) {

        fiber_control_block_t *current =
            find_fiber(current_id);

        if (current == NULL ||
            current->state != FIBER_BLOCKED) {

            return 0;
        }

        fiber_id_t waiting_for =
            current->waiting_for;

        if (waiting_for == 0) {
            return 0;
        }

        if (waiting_for == start_id) {
            return 1;
        }

        current_id = waiting_for;
    }

    return 1;
}

/*
 * Check all blocked fibers for circular dependencies.
 */
static int detect_deadlock(void)
{
    for (unsigned int i = 0; i < MAX_FIBERS; i++) {
        if (fibers[i].state == FIBER_BLOCKED &&
            fibers[i].waiting_for != 0) {

            if (has_wait_cycle(fibers[i].id)) {
                return 1;
            }
        }
    }

    return 0;
}

/*
 * Entry point executed by each fiber.
 */
static void fiber_entry_point(fiber_id_t id)
{
    fiber_control_block_t *fcb =
        find_fiber(id);

    if (fcb == NULL) {
        fprintf(stderr,
                "[FiberLib] Invalid fiber ID.\n");

        return;
    }

    current_fiber = fcb;
    fcb->state = FIBER_RUNNING;

    fcb->function(fcb->arg);

    fcb->state = FIBER_ZOMBIE;

    if (fiber_count > 0) {
        fiber_count--;
    }

    total_completed++;

    printf(
        "[FiberLib] Fiber %u completed execution.\n",
        fcb->id
    );

    wake_waiting_fibers(fcb->id);

    current_fiber = NULL;

    total_context_switches++;

    setcontext(&scheduler_context);
}

/*
 * Voluntarily yield execution to the scheduler.
 */
void fiber_yield(void)
{
    if (!scheduler_running ||
        current_fiber == NULL) {

        return;
    }

    current_fiber->state = FIBER_READY;
    current_fiber->switches++;

    total_yields++;
    total_context_switches++;

    fiber_control_block_t *previous =
        current_fiber;

    current_fiber = NULL;

    if (swapcontext(
            &previous->context,
            &scheduler_context) == -1) {

        perror("[FiberLib] swapcontext");
    }
}

/*
 * Wait for another fiber to complete.
 */
int fiber_join(fiber_id_t fiber_id)
{
    if (!library_initialized) {
        fprintf(stderr,
                "[FiberLib] Error: library is not initialized.\n");

        return -1;
    }

    if (current_fiber == NULL) {
        fprintf(stderr,
                "[FiberLib] Error: fiber_join() must be called from a fiber.\n");

        return -1;
    }

    if (current_fiber->id == fiber_id) {
        fprintf(stderr,
                "[FiberLib] Error: a fiber cannot join itself.\n");

        return -1;
    }

    fiber_control_block_t *target =
        find_fiber_any_state(fiber_id);

    if (target == NULL ||
        target->id == 0) {

        fprintf(stderr,
                "[FiberLib] Error: invalid fiber ID %u.\n",
                fiber_id);

        return -1;
    }

    if (target->state == FIBER_ZOMBIE) {
        return 0;
    }

    current_fiber->waiting_for = fiber_id;
    current_fiber->state = FIBER_BLOCKED;
    current_fiber->aging = 0;

    printf(
        "[FiberLib] Fiber %u is waiting for fiber %u.\n",
        current_fiber->id,
        fiber_id
    );

    /*
     * Detect a circular wait immediately.
     */
    if (detect_deadlock()) {
        deadlock_detected = 1;

        fprintf(
            stderr,
            "[FiberLib] DEADLOCK DETECTED: circular fiber dependency.\n"
        );
    }

    fiber_control_block_t *previous =
        current_fiber;

    current_fiber = NULL;

    total_context_switches++;

    if (swapcontext(
            &previous->context,
            &scheduler_context) == -1) {

        perror("[FiberLib] swapcontext");

        return -1;
    }

    return 0;
}

/*
 * Set the scheduling priority of a fiber.
 */
int fiber_set_priority(
    fiber_id_t fiber_id,
    fiber_priority_t priority)
{
    if (!library_initialized) {
        fprintf(stderr,
                "[FiberLib] Error: library is not initialized.\n");

        return -1;
    }

    if (priority < FIBER_PRIORITY_LOW ||
        priority > FIBER_PRIORITY_HIGH) {

        fprintf(stderr,
                "[FiberLib] Error: invalid priority.\n");

        return -1;
    }

    fiber_control_block_t *fcb =
        find_fiber(fiber_id);

    if (fcb == NULL) {
        fprintf(stderr,
                "[FiberLib] Error: invalid fiber ID %u.\n",
                fiber_id);

        return -1;
    }

    fcb->priority = priority;

    printf(
        "[FiberLib] Fiber %u priority set to %s.\n",
        fiber_id,
        priority_to_string(priority)
    );

    return 0;
}

/*
 * Start the user-space scheduler.
 */
void fiber_schedule(void)
{
    if (!library_initialized) {
        fprintf(stderr,
                "[FiberLib] Error: library is not initialized.\n");

        return;
    }

    if (scheduler_running) {
        fprintf(stderr,
                "[FiberLib] Error: scheduler is already running.\n");

        return;
    }

    scheduler_running = 1;

    deadlock_detected = 0;

    printf("\n[FiberLib] Scheduler started.\n");

    while (fiber_count > 0) {
        fiber_control_block_t *next =
            find_next_ready();

        if (next == NULL) {
            /*
             * No READY fibers remain while active fibers
             * still exist.
             */
            if (detect_deadlock()) {
                deadlock_detected = 1;

                fprintf(
                    stderr,
                    "[FiberLib] DEADLOCK DETECTED: no READY fibers remain.\n"
                );
            } else {
                fprintf(
                    stderr,
                    "[FiberLib] Scheduler stopped: no READY fibers.\n"
                );
            }

            break;
        }

        current_fiber = next;
        next->state = FIBER_RUNNING;
        next->switches++;

        total_dispatches++;
        total_context_switches++;

        if (swapcontext(
                &scheduler_context,
                &next->context) == -1) {

            perror("[FiberLib] swapcontext");

            break;
        }

        current_fiber = NULL;
    }

    scheduler_running = 0;

    printf("[FiberLib] Scheduler finished.\n");
}

/*
 * Return whether the scheduler detected a deadlock
 * during its most recent execution.
 */
int fiber_deadlock_detected(void)
{
    return deadlock_detected;
}

/*
 * Display the current internal state of all active fibers.
 */
void fiber_debug_dump(void)
{
    printf("\n========== FiberLib State ==========\n");

    printf("Library initialized : %s\n",
           library_initialized ? "YES" : "NO");

    printf("Scheduler running   : %s\n",
           scheduler_running ? "YES" : "NO");

    printf("Deadlock detected   : %s\n",
           deadlock_detected ? "YES" : "NO");

    printf("Active fibers       : %u\n",
           fiber_count);

    printf("------------------------------------\n");

    for (unsigned int i = 0;
         i < MAX_FIBERS;
         i++) {

        if (fibers[i].state != FIBER_ZOMBIE) {
            printf(
                "TID: %-3u | State: %-7s | Priority: %-6s | Switches: %-5lu | Aging: %-5lu | Stack: %zu bytes",
                fibers[i].id,
                state_to_string(fibers[i].state),
                priority_to_string(fibers[i].priority),
                fibers[i].switches,
                fibers[i].aging,
                fibers[i].stack_size
            );

            if (fibers[i].state == FIBER_BLOCKED) {
                printf(
                    " | Waiting for: %u",
                    fibers[i].waiting_for
                );
            }

            printf("\n");
        }
    }

    printf("====================================\n\n");
}

/*
 * Display runtime scheduler statistics.
 */
void fiber_stats_dump(void)
{
    printf("\n========== FiberLib Statistics ==========\n");

    printf("Total dispatches        : %lu\n",
           total_dispatches);

    printf("Total yield calls       : %lu\n",
           total_yields);

    printf("Total context switches  : %lu\n",
           total_context_switches);

    printf("Total completed fibers  : %lu\n",
           total_completed);

    printf("Currently active fibers : %u\n",
           fiber_count);

    printf("==========================================\n\n");
}

/*
 * Initialize the FiberLib runtime.
 */
int fiber_library_init(void)
{
    if (library_initialized) {
        return 0;
    }

    initialize_fiber_table();

    if (getcontext(&scheduler_context) == -1) {
        perror("[FiberLib] getcontext");

        return -1;
    }

    library_initialized = 1;

    printf("[FiberLib] Scheduler initialized.\n");

    return 0;
}

/*
 * Create a new fiber.
 */
fiber_id_t fiber_create(
    fiber_function_t function,
    void *arg)
{
    if (!library_initialized) {
        fprintf(stderr,
                "[FiberLib] Error: library is not initialized.\n");

        return 0;
    }

    if (function == NULL) {
        fprintf(stderr,
                "[FiberLib] Error: fiber function cannot be NULL.\n");

        return 0;
    }

    if (fiber_count >= MAX_FIBERS) {
        fprintf(stderr,
                "[FiberLib] Error: maximum fiber limit reached.\n");

        return 0;
    }

    fiber_control_block_t *fcb =
        allocate_fcb();

    if (fcb == NULL) {
        fprintf(stderr,
                "[FiberLib] Error: unable to allocate FCB.\n");

        return 0;
    }

    fcb->stack =
        malloc(FIBER_STACK_SIZE);

    if (fcb->stack == NULL) {
        fprintf(stderr,
                "[FiberLib] Error: unable to allocate fiber stack.\n");

        return 0;
    }

    if (getcontext(&fcb->context) == -1) {
        perror("[FiberLib] getcontext");

        free(fcb->stack);
        fcb->stack = NULL;

        return 0;
    }

    fcb->id = next_fiber_id++;
    fcb->state = FIBER_READY;
    fcb->priority = FIBER_PRIORITY_NORMAL;
    fcb->stack_size = FIBER_STACK_SIZE;
    fcb->function = function;
    fcb->arg = arg;
    fcb->switches = 0;
    fcb->aging = 0;
    fcb->waiting_for = 0;

    fcb->context.uc_stack.ss_sp =
        fcb->stack;

    fcb->context.uc_stack.ss_size =
        fcb->stack_size;

    fcb->context.uc_stack.ss_flags = 0;

    fcb->context.uc_link =
        &scheduler_context;

    makecontext(
        &fcb->context,
        (void (*)(void))fiber_entry_point,
        1,
        fcb->id
    );

    fiber_count++;

    printf(
        "[FiberLib] Created fiber %u | Priority: %s | Stack: %zu bytes\n",
        fcb->id,
        priority_to_string(fcb->priority),
        fcb->stack_size
    );

    return fcb->id;
}

/*
 * Shut down the FiberLib runtime.
 */
void fiber_library_shutdown(void)
{
    if (!library_initialized) {
        return;
    }

    scheduler_running = 0;
    current_fiber = NULL;

    for (unsigned int i = 0;
         i < MAX_FIBERS;
         i++) {

        if (fibers[i].state != FIBER_ZOMBIE) {
            release_fcb(&fibers[i]);
        }
    }

    fiber_count = 0;
    library_initialized = 0;

    printf(
        "[FiberLib] Scheduler shutdown complete.\n"
    );
}
