#include "../include/fiber.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#define FIBER_STACK_SIZE (64 * 1024)
#define MAX_FIBERS 128
#define AGING_THRESHOLD 5
#define MAX_TRACE_EVENTS 1024

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
unsigned long dispatches;
unsigned long yields;
unsigned long aging;

fiber_id_t waiting_for;

} fiber_control_block_t;

static fiber_control_block_t fibers[MAX_FIBERS];

static unsigned int fiber_count = 0;
static fiber_id_t next_fiber_id = 1;

static int scheduler_cursor = -1;

static fiber_scheduler_policy_t scheduler_policy =
FIBER_SCHEDULER_PRIORITY_AGING;

static ucontext_t scheduler_context;

static int library_initialized = 0;
static int scheduler_running = 0;
static int deadlock_detected_flag = 0;

static fiber_control_block_t *current_fiber = NULL;

/* Optional observer; NULL preserves the original behavior. */
static fiber_monitor_callback_t monitor_callback = NULL;

/* Global runtime statistics. */
static unsigned long total_dispatches = 0;
static unsigned long total_yields = 0;
static unsigned long total_context_switches = 0;
static unsigned long total_completed = 0;

/* Scheduler trace storage. */
static fiber_event_t trace_events[MAX_TRACE_EVENTS];
static unsigned long trace_count = 0;
static unsigned long trace_sequence = 0;

/* --------------------------------------------------------- */
/* Utility functions                                         */
/* --------------------------------------------------------- */

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

static const char *event_type_to_string(fiber_event_type_t type)
{
switch (type) {
case FIBER_EVENT_DISPATCH:
return "DISPATCH";

    case FIBER_EVENT_YIELD:
        return "YIELD";

    case FIBER_EVENT_BLOCK:
        return "BLOCK";

    case FIBER_EVENT_RESUME:
        return "RESUME";

    case FIBER_EVENT_COMPLETE:
        return "COMPLETE";

    case FIBER_EVENT_PRIORITY_CHANGE:
        return "PRIORITY";

    case FIBER_EVENT_CONTEXT_SWITCH:
        return "SWITCH";

    default:
        return "UNKNOWN";
}

}

/* --------------------------------------------------------- */
/* Scheduler trace                                           */
/* --------------------------------------------------------- */

void fiber_trace_clear(void)
{
memset(trace_events, 0, sizeof(trace_events));

trace_count = 0;
trace_sequence = 0;

}

static void record_event(
fiber_event_type_t type,
const fiber_control_block_t *fcb
)
{
fiber_event_t event;

if (fcb == NULL) {
    return;
}

memset(&event, 0, sizeof(event));

event.sequence = ++trace_sequence;
event.type = type;
event.fiber_id = fcb->id;
event.state = fcb->state;
event.priority = fcb->priority;
event.dispatches = total_dispatches;
event.yields = total_yields;
event.switches = total_context_switches;
event.aging = (unsigned int)fcb->aging;

if (trace_count < MAX_TRACE_EVENTS) {
    trace_events[trace_count] = event;
    trace_count++;
} else {
    /* Keep the newest events when the trace buffer is full. */
    memmove(
        &trace_events[0],
        &trace_events[1],
        sizeof(fiber_event_t) * (MAX_TRACE_EVENTS - 1)
    );

    trace_events[MAX_TRACE_EVENTS - 1] = event;
}

}

unsigned long fiber_trace_count(void)
{
return trace_count;
}

int fiber_trace_get(
unsigned long index,
fiber_event_t *event
)
{
if (event == NULL || index >= trace_count) {
return -1;
}

*event = trace_events[index];

return 0;

}

void fiber_trace_dump(void)
{
unsigned long i;

printf("\n========== FiberLib Scheduler Trace ==========\n");

printf(
    "SEQ    EVENT        TID   STATE     PRIORITY   "
    "DISP    YIELD   SWITCH  AGE\n"
);

printf(
    "-------------------------------------------------------------------------------\n"
);

for (i = 0; i < trace_count; i++) {
    const fiber_event_t *event = &trace_events[i];

    printf(
        "%-6lu %-12s %-5u %-9s %-10s %-7lu %-7lu %-7lu %-5u\n",
        event->sequence,
        event_type_to_string(event->type),
        event->fiber_id,
        state_to_string(event->state),
        priority_to_string(event->priority),
        event->dispatches,
        event->yields,
        event->switches,
        event->aging
    );
}

printf(
    "-------------------------------------------------------------------------------\n"
);

printf(
    "Recorded events: %lu / %d\n",
    trace_count,
    MAX_TRACE_EVENTS
);

printf(
    "==============================================\n"
);

}

/* --------------------------------------------------------- */
/* Fiber table                                               */
/* --------------------------------------------------------- */

static void initialize_fiber_table(void)
{
memset(fibers, 0, sizeof(fibers));

fiber_count = 0;
next_fiber_id = 1;
scheduler_cursor = -1;

/* Reset to the original default scheduling policy. */
scheduler_policy = FIBER_SCHEDULER_PRIORITY_AGING;

current_fiber = NULL;

total_dispatches = 0;
total_yields = 0;
total_context_switches = 0;
total_completed = 0;

deadlock_detected_flag = 0;

fiber_trace_clear();

}

/* --------------------------------------------------------- */
/* Fiber lookup                                              */
/* --------------------------------------------------------- */

static fiber_control_block_t *find_fiber(fiber_id_t id)
{
unsigned int i;

for (i = 0; i < MAX_FIBERS; i++) {
    if (fibers[i].id == id &&
        fibers[i].state != FIBER_ZOMBIE) {
        return &fibers[i];
    }
}

return NULL;

}

static fiber_control_block_t *find_fiber_any_state(fiber_id_t id)
{
unsigned int i;

for (i = 0; i < MAX_FIBERS; i++) {
    if (fibers[i].id == id) {
        return &fibers[i];
    }
}

return NULL;

}

/* --------------------------------------------------------- */
/* Fiber allocation                                          */
/* --------------------------------------------------------- */

static fiber_control_block_t *allocate_fcb(void)
{
unsigned int i;

for (i = 0; i < MAX_FIBERS; i++) {
    if (fibers[i].id == 0) {
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

memset(fcb, 0, sizeof(*fcb));

}

/* --------------------------------------------------------- */
/* Priority and aging                                        */
/* --------------------------------------------------------- */

static int effective_priority(const fiber_control_block_t *fcb)
{
int priority;

if (fcb == NULL) {
    return -1;
}

priority = (int)fcb->priority;

if (fcb->aging >= AGING_THRESHOLD) {
    priority++;
}

if (fcb->aging >= (AGING_THRESHOLD * 2)) {
    priority++;
}

if (priority > FIBER_PRIORITY_HIGH) {
    priority = FIBER_PRIORITY_HIGH;
}

return priority;

}

/* --------------------------------------------------------- */
/* Scheduler selection                                       */
/* --------------------------------------------------------- */

static fiber_control_block_t *find_next_ready(void)
{
int best_priority = -1;
fiber_control_block_t *selected = NULL;
unsigned int offset;

/*
 * Scan cyclically from the cursor.
 *
 * Round-Robin:
 *     Return the first READY fiber encountered.
 *
 * Priority + Aging:
 *     Select the READY fiber with the highest effective
 *     priority. Cyclic scan order breaks priority ties.
 */
for (offset = 1; offset <= MAX_FIBERS; offset++) {
    int index;
    fiber_control_block_t *candidate;

    index = (scheduler_cursor + (int)offset) % MAX_FIBERS;
    candidate = &fibers[index];

    if (candidate->id == 0) {
        continue;
    }

    if (candidate->state != FIBER_READY) {
        continue;
    }

    if (scheduler_policy == FIBER_SCHEDULER_ROUND_ROBIN) {
        return candidate;
    }

    {
        int priority = effective_priority(candidate);

        if (priority > best_priority) {
            best_priority = priority;
            selected = candidate;
        }
    }
}

return selected;

}

/* --------------------------------------------------------- */
/* Waiting / join handling                                   */
/* --------------------------------------------------------- */

static void wake_waiting_fibers(fiber_id_t completed_id)
{
unsigned int i;

for (i = 0; i < MAX_FIBERS; i++) {
    if (fibers[i].id == 0) {
        continue;
    }

    if (fibers[i].state == FIBER_BLOCKED &&
        fibers[i].waiting_for == completed_id) {

        fibers[i].state = FIBER_READY;
        fibers[i].waiting_for = 0;
        fibers[i].aging = 0;

        record_event(
            FIBER_EVENT_RESUME,
            &fibers[i]
        );
    }
}

}

static int has_wait_cycle(
    fiber_control_block_t *start,
    fiber_control_block_t *current
)
{
    fiber_control_block_t *visited[MAX_FIBERS];
    unsigned int visited_count = 0;

    if (start == NULL || current == NULL) {
        return 0;
    }

    while (current != NULL) {
        fiber_control_block_t *target;
        unsigned int i;

        if (current->waiting_for == 0) {
            return 0;
        }

        if (current->waiting_for == start->id) {
            return 1;
        }

        target = find_fiber(current->waiting_for);

        if (target == NULL) {
            return 0;
        }

        for (i = 0; i < visited_count; i++) {
            if (visited[i] == target) {
                return 0;
            }
        }

        if (visited_count >= MAX_FIBERS) {
            return 0;
        }

        visited[visited_count++] = current;
        current = target;
    }

    return 0;
}

static int detect_deadlock(void)
{
unsigned int i;

for (i = 0; i < MAX_FIBERS; i++) {
    if (fibers[i].id == 0) {
        continue;
    }

    if (fibers[i].state == FIBER_BLOCKED) {
        if (has_wait_cycle(&fibers[i], &fibers[i])) {
            return 1;
        }
    }
}

return 0;

}

/* --------------------------------------------------------- */
/* Fiber entry point                                         */
/* --------------------------------------------------------- */

static void fiber_entry_point(fiber_id_t fiber_id)
{
fiber_control_block_t *fcb;

fcb = find_fiber_any_state(fiber_id);

if (fcb == NULL) {
    setcontext(&scheduler_context);
    return;
}

current_fiber = fcb;

record_event(
    FIBER_EVENT_RESUME,
    fcb
);

if (fcb->function != NULL) {
    fcb->function(fcb->arg);
}

fcb->state = FIBER_ZOMBIE;

if (fiber_count > 0) {
    fiber_count--;
}

total_completed++;

record_event(
    FIBER_EVENT_COMPLETE,
    fcb
);

printf(
    "[FiberLib] Fiber %u completed execution.\n",
    fcb->id
);

wake_waiting_fibers(fcb->id);

current_fiber = NULL;

setcontext(&scheduler_context);

}

/* --------------------------------------------------------- */
/* Fiber yield                                               */
/* --------------------------------------------------------- */

void fiber_yield(void)
{
fiber_control_block_t *fcb;

if (!library_initialized) {
    printf(
        "[FiberLib] Error: library is not initialized.\n"
    );
    return;
}

if (!scheduler_running || current_fiber == NULL) {
    return;
}

fcb = current_fiber;

fcb->state = FIBER_READY;
fcb->aging = 0;

total_yields++;
fcb->yields++;

fcb->switches++;

total_context_switches++;

record_event(
    FIBER_EVENT_YIELD,
    fcb
);

record_event(
    FIBER_EVENT_CONTEXT_SWITCH,
    fcb
);

if (swapcontext(
        &fcb->context,
        &scheduler_context
    ) == -1) {

    perror(
        "[FiberLib] swapcontext"
    );
}

}

/* --------------------------------------------------------- */
/* Fiber join                                                */
/* --------------------------------------------------------- */

int fiber_join(fiber_id_t fiber_id)
{
fiber_control_block_t *target;

if (!library_initialized) {
    printf(
        "[FiberLib] Error: library is not initialized.\n"
    );
    return -1;
}

if (current_fiber == NULL) {
    printf(
        "[FiberLib] Error: join must be called from a fiber.\n"
    );
    return -1;
}

if (fiber_id == current_fiber->id) {
    printf(
        "[FiberLib] Error: a fiber cannot join itself.\n"
    );
    return -1;
}

target = find_fiber_any_state(fiber_id);

if (target == NULL) {
    printf(
        "[FiberLib] Error: invalid fiber ID %u.\n",
        fiber_id
    );
    return -1;
}

if (target->state == FIBER_ZOMBIE) {
    return 0;
}

current_fiber->state = FIBER_BLOCKED;
current_fiber->waiting_for = fiber_id;

record_event(
    FIBER_EVENT_BLOCK,
    current_fiber
);

if (detect_deadlock()) {
    deadlock_detected_flag = 1;

    printf(
        "[FiberLib] DEADLOCK DETECTED: "
        "circular fiber dependency.\n"
    );

    current_fiber->state = FIBER_BLOCKED;
    return -1;
}

total_context_switches++;

current_fiber->switches++;

record_event(
    FIBER_EVENT_CONTEXT_SWITCH,
    current_fiber
);

if (swapcontext(
        &current_fiber->context,
        &scheduler_context
    ) == -1) {

    perror(
        "[FiberLib] swapcontext"
    );

    return -1;
}

return 0;

}

/* --------------------------------------------------------- */
/* Priority configuration                                    */
/* --------------------------------------------------------- */

int fiber_set_priority(
fiber_id_t fiber_id,
fiber_priority_t priority
)
{
fiber_control_block_t *fcb;

if (!library_initialized) {
    printf(
        "[FiberLib] Error: library is not initialized.\n"
    );
    return -1;
}

if (priority < FIBER_PRIORITY_LOW ||
    priority > FIBER_PRIORITY_HIGH) {

    printf(
        "[FiberLib] Error: invalid priority.\n"
    );

    return -1;
}

fcb = find_fiber_any_state(fiber_id);

if (fcb == NULL || fcb->state == FIBER_ZOMBIE) {
    printf(
        "[FiberLib] Error: invalid fiber ID %u.\n",
        fiber_id
    );

    return -1;
}

fcb->priority = priority;

record_event(
    FIBER_EVENT_PRIORITY_CHANGE,
    fcb
);

printf(
    "[FiberLib] Fiber %u priority set to %s.\n",
    fiber_id,
    priority_to_string(priority)
);

return 0;

}

/* --------------------------------------------------------- */
/* Scheduler policy configuration                            */
/* --------------------------------------------------------- */

int fiber_set_scheduler_policy(fiber_scheduler_policy_t policy)
{
if (!library_initialized) {
printf(
"[FiberLib] Error: library is not initialized.\n"
);
return -1;
}

if (scheduler_running) {
    printf(
        "[FiberLib] Error: cannot change scheduling policy "
        "while the scheduler is running.\n"
    );
    return -1;
}

if (policy != FIBER_SCHEDULER_PRIORITY_AGING &&
    policy != FIBER_SCHEDULER_ROUND_ROBIN) {

    printf(
        "[FiberLib] Error: invalid scheduling policy.\n"
    );
    return -1;
}

scheduler_policy = policy;

printf(
    "[FiberLib] Scheduling policy set to %s.\n",
    scheduler_policy == FIBER_SCHEDULER_ROUND_ROBIN
        ? "Round-Robin"
        : "Priority + Aging"
);

return 0;

}

fiber_scheduler_policy_t fiber_get_scheduler_policy(void)
{
return scheduler_policy;
}

/* --------------------------------------------------------- */
/* Scheduler                                                 */
/* --------------------------------------------------------- */

void fiber_set_monitor_callback(fiber_monitor_callback_t callback)
{
    monitor_callback = callback;
}

void fiber_schedule(void)
{
fiber_control_block_t *next;

if (!library_initialized) {
    printf(
        "[FiberLib] Error: library is not initialized.\n"
    );
    return;
}

if (scheduler_running) {
    return;
}

scheduler_running = 1;

deadlock_detected_flag = 0;

printf(
    "\n[FiberLib] Scheduler started using %s policy.\n",
    scheduler_policy == FIBER_SCHEDULER_ROUND_ROBIN
        ? "Round-Robin"
        : "Priority + Aging"
);

while (fiber_count > 0) {
    next = find_next_ready();

    if (next == NULL) {
        if (detect_deadlock()) {
            deadlock_detected_flag = 1;

            printf(
                "[FiberLib] DEADLOCK DETECTED: "
                "no READY fibers remain.\n"
            );
        }

        break;
    }

    scheduler_cursor = (int)(next - fibers);

    next->state = FIBER_RUNNING;
    current_fiber = next;

    total_dispatches++;
    next->dispatches++;

    next->switches++;

    total_context_switches++;

    record_event(
        FIBER_EVENT_DISPATCH,
        next
    );

    record_event(
        FIBER_EVENT_CONTEXT_SWITCH,
        next
    );

    /*
     * Aging applies only to Priority + Aging.
     * Round-Robin does not use priority or aging.
     */
    if (scheduler_policy == FIBER_SCHEDULER_PRIORITY_AGING) {
        unsigned int i;

        for (i = 0; i < MAX_FIBERS; i++) {
            if (fibers[i].id == 0) {
                continue;
            }

            if (&fibers[i] == next) {
                fibers[i].aging = 0;
            } else if (fibers[i].state == FIBER_READY) {
                fibers[i].aging++;
            }
        }
    } else {
        next->aging = 0;
    }

    if (monitor_callback != NULL) {
        monitor_callback();
    }

    if (swapcontext(
            &scheduler_context,
            &next->context
        ) == -1) {

        perror(
            "[FiberLib] swapcontext"
        );

        break;
    }

    if (monitor_callback != NULL) {
        monitor_callback();
    }
}

current_fiber = NULL;

scheduler_running = 0;

printf(
    "[FiberLib] Scheduler finished.\n"
);

}

/* --------------------------------------------------------- */
/* Public state/statistics                                   */
/* --------------------------------------------------------- */

int fiber_deadlock_detected(void)
{
return deadlock_detected_flag;
}

int fiber_get_snapshot(
    fiber_id_t fiber_id,
    fiber_snapshot_t *snapshot
)
{
    fiber_control_block_t *fcb;

    if (!library_initialized || snapshot == NULL ||
        fiber_id == 0) {
        return -1;
    }

    fcb = find_fiber_any_state(fiber_id);
    if (fcb == NULL) {
        return -1;
    }

    snapshot->id = fcb->id;
    snapshot->state = fcb->state;
    snapshot->priority = fcb->priority;
    snapshot->dispatches = fcb->dispatches;
    snapshot->yields = fcb->yields;
    snapshot->switches = fcb->switches;
    snapshot->aging = fcb->aging;
    snapshot->waiting_for = fcb->waiting_for;

    return 0;
}

void fiber_debug_dump(void)
{
unsigned int i;

printf(
    "\n========== FiberLib State ==========\n"
);

printf(
    "Library initialized : %s\n",
    library_initialized ? "YES" : "NO"
);

printf(
    "Scheduler running   : %s\n",
    scheduler_running ? "YES" : "NO"
);

printf(
    "Scheduling policy   : %s\n",
    scheduler_policy == FIBER_SCHEDULER_ROUND_ROBIN
        ? "Round-Robin"
        : "Priority + Aging"
);

printf(
    "Deadlock detected   : %s\n",
    deadlock_detected_flag ? "YES" : "NO"
);

printf(
    "Active fibers       : %u\n",
    fiber_count
);

printf(
    "------------------------------------\n"
);

for (i = 0; i < MAX_FIBERS; i++) {
    if (fibers[i].id == 0) {
        continue;
    }

    printf(
        "TID: %-4u | State: %-8s | Priority: %-6s | "
        "Switches: %-5lu | Aging: %-5lu | "
        "Stack: %-6zu bytes",
        fibers[i].id,
        state_to_string(fibers[i].state),
        priority_to_string(fibers[i].priority),
        fibers[i].switches,
        fibers[i].aging,
        fibers[i].stack_size
    );

    if (fibers[i].waiting_for != 0) {
        printf(
            " | Waiting for: %u",
            fibers[i].waiting_for
        );
    }

    printf("\n");
}

printf(
    "====================================\n"
);

}

void fiber_stats_dump(void)
{
printf(
"\n========== FiberLib Statistics ==========\n"
);

printf(
    "Scheduling policy       : %s\n",
    scheduler_policy == FIBER_SCHEDULER_ROUND_ROBIN
        ? "Round-Robin"
        : "Priority + Aging"
);

printf(
    "Total dispatches        : %lu\n",
    total_dispatches
);

printf(
    "Total yields            : %lu\n",
    total_yields
);

printf(
    "Total context switches  : %lu\n",
    total_context_switches
);

printf(
    "Total completed fibers  : %lu\n",
    total_completed
);

printf(
    "Trace events recorded   : %lu\n",
    trace_count
);

printf(
    "==========================================\n"
);

}

/* --------------------------------------------------------- */
/* Library initialization                                    */
/* --------------------------------------------------------- */

int fiber_library_init(void)
{
if (library_initialized) {
return 0;
}

initialize_fiber_table();

library_initialized = 1;

printf(
    "[FiberLib] Scheduler initialized.\n"
);

return 0;

}

/* --------------------------------------------------------- */
/* Fiber creation                                            */
/* --------------------------------------------------------- */

fiber_id_t fiber_create(
fiber_function_t function,
void *arg
)
{
fiber_control_block_t *fcb;

if (!library_initialized) {
    printf(
        "[FiberLib] Error: library is not initialized.\n"
    );
    return 0;
}

if (function == NULL) {
    printf(
        "[FiberLib] Error: fiber function cannot be NULL.\n"
    );
    return 0;
}

if (fiber_count >= MAX_FIBERS) {
    printf(
        "[FiberLib] Error: maximum fiber limit reached.\n"
    );
    return 0;
}

fcb = allocate_fcb();

if (fcb == NULL) {
    printf(
        "[FiberLib] Error: unable to allocate fiber control block.\n"
    );
    return 0;
}

memset(
    fcb,
    0,
    sizeof(*fcb)
);

fcb->id = next_fiber_id++;

fcb->state = FIBER_READY;

fcb->priority = FIBER_PRIORITY_NORMAL;

fcb->stack_size = FIBER_STACK_SIZE;

fcb->function = function;

fcb->arg = arg;

fcb->waiting_for = 0;

fcb->switches = 0;

fcb->aging = 0;

fcb->stack = malloc(
    fcb->stack_size
);

if (fcb->stack == NULL) {
    printf(
        "[FiberLib] Error: stack allocation failed.\n"
    );

    release_fcb(fcb);

    return 0;
}

if (getcontext(&fcb->context) == -1) {
    perror(
        "[FiberLib] getcontext"
    );

    release_fcb(fcb);

    return 0;
}

fcb->context.uc_stack.ss_sp = fcb->stack;

fcb->context.uc_stack.ss_size = fcb->stack_size;

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

/* --------------------------------------------------------- */
/* Library shutdown                                          */
/* --------------------------------------------------------- */

void fiber_library_shutdown(void)
{
unsigned int i;

if (!library_initialized) {
    return;
}

for (i = 0; i < MAX_FIBERS; i++) {
    if (fibers[i].id != 0) {
        if (fibers[i].stack != NULL) {
            free(fibers[i].stack);
        }

        fibers[i].stack = NULL;
    }
}

initialize_fiber_table();

library_initialized = 0;

scheduler_running = 0;

current_fiber = NULL;

printf(
    "[FiberLib] Scheduler shutdown complete.\n"
);

}
