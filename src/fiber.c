#include "../include/fiber.h"
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIBER_STACK_SIZE (64 * 1024)
#define MAX_FIBERS 128

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

    fiber_id_t waiting_for;
} fiber_control_block_t;

static fiber_control_block_t fibers[MAX_FIBERS];

static unsigned int fiber_count = 0;
static fiber_id_t next_fiber_id = 1;

static unsigned int scheduler_cursor = 0;

static ucontext_t scheduler_context;

static int library_initialized = 0;
static int scheduler_running = 0;

static fiber_control_block_t *current_fiber = NULL;

/*
 * Global scheduler statistics.
 */
static unsigned long total_dispatches = 0;
static unsigned long total_yields = 0;
static unsigned long total_context_switches = 0;
static unsigned long total_completed = 0;

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

static void initialize_fiber_table(void)
{
    memset(fibers, 0, sizeof(fibers));

    for (int i = 0; i < MAX_FIBERS; i++) {
        fibers[i].state = FIBER_ZOMBIE;
    }

    fiber_count = 0;
    next_fiber_id = 1;
    scheduler_cursor = 0;

    total_dispatches = 0;
    total_yields = 0;
    total_context_switches = 0;
    total_completed = 0;
}

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

static fiber_control_block_t *find_fiber_any_state(fiber_id_t id)
{
    for (unsigned int i = 0; i < MAX_FIBERS; i++) {
        if (fibers[i].id == id) {
            return &fibers[i];
        }
    }

    return NULL;
}

static fiber_control_block_t *allocate_fcb(void)
{
    for (unsigned int i = 0; i < MAX_FIBERS; i++) {
        if (fibers[i].state == FIBER_ZOMBIE) {
            return &fibers[i];
        }
    }

    return NULL;
}

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
    fcb->waiting_for = 0;
    fcb->priority = FIBER_PRIORITY_NORMAL;
    fcb->state = FIBER_ZOMBIE;
    fcb->id = 0;
}

/*
 * Priority-aware Round-Robin scheduler.
 *
 * The scheduler first finds the highest-priority READY fiber.
 * Within the same priority level, fibers are selected in
 * Round-Robin order using scheduler_cursor.
 */
static fiber_control_block_t *find_next_ready(void)
{
    fiber_priority_t highest_priority = 0;

    /*
     * Find the highest priority among READY fibers.
     */
    for (unsigned int i = 0; i < MAX_FIBERS; i++) {
        if (fibers[i].state == FIBER_READY &&
            fibers[i].priority > highest_priority) {

            highest_priority = fibers[i].priority;
        }
    }

    if (highest_priority == 0) {
        return NULL;
    }

    /*
     * Select a fiber with the highest priority,
     * starting from the scheduler cursor.
     */
    for (unsigned int offset = 0; offset < MAX_FIBERS; offset++) {
        unsigned int index =
            (scheduler_cursor + offset) % MAX_FIBERS;

        if (fibers[index].state == FIBER_READY &&
            fibers[index].priority == highest_priority) {

            scheduler_cursor = (index + 1) % MAX_FIBERS;

            return &fibers[index];
        }
    }

    return NULL;
}

static void wake_waiting_fibers(fiber_id_t completed_id)
{
    for (unsigned int i = 0; i < MAX_FIBERS; i++) {
        if (fibers[i].state == FIBER_BLOCKED &&
            fibers[i].waiting_for == completed_id) {

            fibers[i].state = FIBER_READY;
            fibers[i].waiting_for = 0;

            printf(
                "[FiberLib] Fiber %u unblocked after fiber %u finished.\n",
                fibers[i].id,
                completed_id
            );
        }
    }
}

static void fiber_entry_point(fiber_id_t id)
{
    fiber_control_block_t *fcb = find_fiber(id);

    if (fcb == NULL) {
        fprintf(stderr, "[FiberLib] Invalid fiber ID.\n");
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

void fiber_yield(void)
{
    if (!scheduler_running || current_fiber == NULL) {
        return;
    }

    current_fiber->state = FIBER_READY;
    current_fiber->switches++;

    total_yields++;
    total_context_switches++;

    fiber_control_block_t *previous = current_fiber;

    current_fiber = NULL;

    if (swapcontext(&previous->context, &scheduler_context) == -1) {
        perror("[FiberLib] swapcontext");
    }
}

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

    fiber_control_block_t *target = find_fiber_any_state(fiber_id);

    if (target == NULL || target->id == 0) {
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

    printf(
        "[FiberLib] Fiber %u is waiting for fiber %u.\n",
        current_fiber->id,
        fiber_id
    );

    fiber_control_block_t *previous = current_fiber;

    current_fiber = NULL;

    total_context_switches++;

    if (swapcontext(&previous->context, &scheduler_context) == -1) {
        perror("[FiberLib] swapcontext");
        return -1;
    }

    return 0;
}

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

    fiber_control_block_t *fcb = find_fiber(fiber_id);

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

    printf("\n[FiberLib] Scheduler started.\n");

    while (fiber_count > 0) {
        fiber_control_block_t *next = find_next_ready();

        if (next == NULL) {
            fprintf(stderr,
                    "[FiberLib] Scheduler stopped: no READY fibers.\n");
            break;
        }

        current_fiber = next;
        next->state = FIBER_RUNNING;
        next->switches++;

        total_dispatches++;
        total_context_switches++;

        if (swapcontext(&scheduler_context, &next->context) == -1) {
            perror("[FiberLib] swapcontext");
            break;
        }

        current_fiber = NULL;
    }

    scheduler_running = 0;

    printf("[FiberLib] Scheduler finished.\n");
}

void fiber_debug_dump(void)
{
    printf("\n========== FiberLib State ==========\n");

    printf("Library initialized : %s\n",
           library_initialized ? "YES" : "NO");

    printf("Scheduler running   : %s\n",
           scheduler_running ? "YES" : "NO");

    printf("Active fibers       : %u\n", fiber_count);

    printf("------------------------------------\n");

    for (unsigned int i = 0; i < MAX_FIBERS; i++) {
        if (fibers[i].state != FIBER_ZOMBIE) {
            printf(
                "TID: %-3u | State: %-7s | Priority: %-6s | Switches: %-5lu | Stack: %zu bytes",
                fibers[i].id,
                state_to_string(fibers[i].state),
                priority_to_string(fibers[i].priority),
                fibers[i].switches,
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

fiber_id_t fiber_create(fiber_function_t function, void *arg)
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

    fiber_control_block_t *fcb = allocate_fcb();

    if (fcb == NULL) {
        fprintf(stderr,
                "[FiberLib] Error: unable to allocate FCB.\n");
        return 0;
    }

    fcb->stack = malloc(FIBER_STACK_SIZE);

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
    fcb->waiting_for = 0;

    fcb->context.uc_stack.ss_sp = fcb->stack;
    fcb->context.uc_stack.ss_size = fcb->stack_size;
    fcb->context.uc_stack.ss_flags = 0;

    fcb->context.uc_link = &scheduler_context;

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

void fiber_library_shutdown(void)
{
    if (!library_initialized) {
        return;
    }

    scheduler_running = 0;
    current_fiber = NULL;

    for (unsigned int i = 0; i < MAX_FIBERS; i++) {
        if (fibers[i].state != FIBER_ZOMBIE) {
            release_fcb(&fibers[i]);
        }
    }

    fiber_count = 0;
    library_initialized = 0;

    printf("[FiberLib] Scheduler shutdown complete.\n");
}
