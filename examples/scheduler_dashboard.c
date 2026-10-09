#include "../include/fiber.h"

#include <stdio.h>
#include <stdlib.h>

#define MAX_DISPLAY_EVENTS 1024
#define TIMELINE_WIDTH 60

static void network_worker(void *arg)
{
    (void)arg;
    fiber_yield();
    fiber_yield();
}

static void logger_worker(void *arg)
{
    (void)arg;
    fiber_yield();
    fiber_yield();
}

static void monitor_worker(void *arg)
{
    (void)arg;
    fiber_yield();
}

static const char *event_name(fiber_event_type_t type)
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

static const char *state_name(fiber_state_t state)
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

static const char *priority_name(fiber_priority_t priority)
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

static void live_dashboard_refresh(void)
{
    unsigned long trace_count;
    unsigned long dispatches = 0;
    unsigned long yields = 0;
    unsigned long switches = 0;
    unsigned long completed = 0;
    unsigned long i;

    /* Clear the terminal and redraw from the scheduler context. */
    printf("\033[2J\033[H");
    printf("============================================================\n");
    printf("             FIBERLIB LIVE SCHEDULER DASHBOARD\n");
    printf("============================================================\n");
    printf("Policy: %-22s  Refresh: scheduler checkpoint\n",
           fiber_get_scheduler_policy() ==
               FIBER_SCHEDULER_ROUND_ROBIN
               ? "Round-Robin"
               : "Priority + Aging");

    printf("------------------------------------------------------------\n");
    printf("%-7s %-10s %-10s %-10s %-8s %-7s\n",
           "FIBER", "STATE", "PRIORITY", "DISPATCH", "YIELDS", "AGE");
    printf("------------------------------------------------------------\n");

    for (i = 1; i <= 128; i++) {
        fiber_snapshot_t snapshot;

        if (fiber_get_snapshot((fiber_id_t)i, &snapshot) != 0) {
            continue;
        }

        printf("F%-6u %-10s %-10s %-10lu %-8lu %-7lu\n",
               snapshot.id,
               state_name(snapshot.state),
               priority_name(snapshot.priority),
               snapshot.dispatches,
               snapshot.yields,
               snapshot.aging);

        dispatches += snapshot.dispatches;
        yields += snapshot.yields;
        switches += snapshot.switches;

        if (snapshot.state == FIBER_ZOMBIE) {
            completed++;
        }
    }

    trace_count = fiber_trace_count();

    printf("------------------------------------------------------------\n");
    printf("Dispatches: %-8lu Yields: %-8lu Switches: %-8lu\n",
           dispatches, yields, switches);
    printf("Completed fibers: %-5lu Trace events: %-5lu\n",
           completed, trace_count);

    if (trace_count > 0) {
        fiber_event_t event;

        if (fiber_trace_get(trace_count - 1, &event) == 0) {
            printf("Latest event: #%-5lu %-12s F%u (%s)\n",
                   event.sequence,
                   event_name(event.type),
                   event.fiber_id,
                   state_name(event.state));
        }
    }

    printf("============================================================\n");
    printf("Live refresh occurs between fiber executions.\n");
    printf("The final reports remain available after scheduling.\n");
    fflush(stdout);
}

static void print_header(void)
{
    printf("\n");
    printf("+----------------------------------------------------------+\n");
    printf("|              FIBERLIB SCHEDULER DASHBOARD                |\n");
    printf("+----------------------------------------------------------+\n");
}

static void print_timeline(
    const unsigned long *fiber_dispatches,
    unsigned long fiber_count)
{
    unsigned long total = 0;
    unsigned long i;
    unsigned long shown;

    for (i = 0; i < fiber_count; i++) {
        total += fiber_dispatches[i];
    }

    printf("\nEXECUTION SUMMARY\n");
    printf("------------------------------------------------------------\n");
    printf("Total dispatches recorded: %lu\n", total);

    if (total == 0) {
        printf("No dispatch events were recorded.\n");
        return;
    }

    printf("\nDISPATCH DISTRIBUTION\n");

    for (i = 0; i < fiber_count; i++) {
        unsigned long bar_length;
        unsigned long j;
        double percentage;

        if (fiber_dispatches[i] == 0) {
            continue;
        }

        percentage =
            (double)fiber_dispatches[i] * 100.0 / (double)total;

        bar_length =
            (unsigned long)(percentage / 2.0 + 0.5);

        printf("Fiber %-3lu | ", i + 1);

        for (j = 0; j < bar_length; j++) {
            putchar('#');
        }

        printf(" %5.1f%% (%lu dispatches)\n",
               percentage, fiber_dispatches[i]);
    }

    printf("\nDISPATCH TIMELINE\n");

    shown = total < TIMELINE_WIDTH ? total : TIMELINE_WIDTH;

    printf("First %lu dispatches: ", shown);

    {
        unsigned long event_count = fiber_trace_count();
        unsigned long dispatch_index = 0;

        for (i = 0; i < event_count && dispatch_index < shown; i++) {
            fiber_event_t event;

            if (fiber_trace_get(i, &event) != 0) {
                continue;
            }

            if (event.type != FIBER_EVENT_DISPATCH) {
                continue;
            }

            printf("F%u", event.fiber_id);
            dispatch_index++;

            if (dispatch_index < shown) {
                printf(" -> ");
            }
        }
    }

    printf("\n");
}

static void show_dashboard(void)
{
    unsigned long event_count = fiber_trace_count();
    unsigned long dispatches[128] = {0};
    unsigned long yields[128] = {0};
    unsigned long switches[128] = {0};
    unsigned long completions[128] = {0};
    unsigned long total_dispatches = 0;
    unsigned long total_yields = 0;
    unsigned long total_switches = 0;
    unsigned long total_completions = 0;
    unsigned long i;

    if (event_count > MAX_DISPLAY_EVENTS) {
        event_count = MAX_DISPLAY_EVENTS;
    }

    for (i = 0; i < event_count; i++) {
        fiber_event_t event;
        unsigned long index;

        if (fiber_trace_get(i, &event) != 0) {
            continue;
        }

        if (event.fiber_id == 0 || event.fiber_id > 128) {
            continue;
        }

        index = (unsigned long)event.fiber_id - 1;

        switch (event.type) {
            case FIBER_EVENT_DISPATCH:
                dispatches[index]++;
                total_dispatches++;
                break;

            case FIBER_EVENT_YIELD:
                yields[index]++;
                total_yields++;
                break;

            case FIBER_EVENT_CONTEXT_SWITCH:
                switches[index]++;
                total_switches++;
                break;

            case FIBER_EVENT_COMPLETE:
                completions[index]++;
                total_completions++;
                break;

            default:
                break;
        }
    }

    print_header();

    printf("| Trace events: %-6lu                                     |\n",
           event_count);
    printf("+----------------------------------------------------------+\n");

    printf("\nPER-FIBER ACTIVITY\n");
    printf("------------------------------------------------------------\n");
    printf("%-7s %-10s %-10s %-10s %-10s\n",
           "FIBER", "DISPATCH", "YIELDS", "SWITCHES", "COMPLETED");
    printf("------------------------------------------------------------\n");

    for (i = 0; i < 128; i++) {
        if (dispatches[i] == 0 &&
            yields[i] == 0 &&
            switches[i] == 0 &&
            completions[i] == 0) {
            continue;
        }

        printf("F%-6lu %-10lu %-10lu %-10lu %-10lu\n",
               i + 1,
               dispatches[i],
               yields[i],
               switches[i],
               completions[i]);
    }

    printf("------------------------------------------------------------\n");
    printf("Total dispatches : %lu\n", total_dispatches);
    printf("Total yields     : %lu\n", total_yields);
    printf("Context switches : %lu\n", total_switches);
    printf("Completions      : %lu\n", total_completions);

    print_timeline(dispatches, 128);

    printf("\nRECENT SCHEDULER EVENTS\n");
    printf("------------------------------------------------------------\n");
    printf("%-5s %-12s %-7s %-10s %-8s\n",
           "SEQ", "EVENT", "FIBER", "STATE", "PRIORITY");
    printf("------------------------------------------------------------\n");

    {
        unsigned long start =
            event_count > 15 ? event_count - 15 : 0;

        for (i = start; i < event_count; i++) {
            fiber_event_t event;

            if (fiber_trace_get(i, &event) != 0) {
                continue;
            }

            printf("%-5lu %-12s F%-6u %-10s %-8s\n",
                   event.sequence,
                   event_name(event.type),
                   event.fiber_id,
                   state_name(event.state),
                   priority_name(event.priority));
        }
    }

    printf("------------------------------------------------------------\n");
}

int main(void)
{
    int choice;

    printf("\n");
    printf("============================================================\n");
    printf("             FiberLib Scheduler Laboratory\n");
    printf("============================================================\n");

    if (fiber_library_init() != 0) {
        fprintf(stderr, "Failed to initialize FiberLib.\n");
        return EXIT_FAILURE;
    }

    fiber_create(network_worker, NULL);
    fiber_create(logger_worker, NULL);
    fiber_create(monitor_worker, NULL);

    printf("\nWorkload created: Network, Logger, and Monitor fibers.\n");
    printf("Starting the live cooperative scheduler...\n\n");

    fiber_set_monitor_callback(live_dashboard_refresh);
    fiber_schedule();
    fiber_set_monitor_callback(NULL);

    printf("\nScheduler execution finished.\n");

    do {
        printf("\n+--------------------------------------+\n");
        printf("|          SCHEDULER LAB MENU          |\n");
        printf("+--------------------------------------+\n");
        printf("| 1. View Scheduler Dashboard          |\n");
        printf("| 2. View Complete Event Trace         |\n");
        printf("| 3. View Library Statistics            |\n");
        printf("| 4. Exit                               |\n");
        printf("+--------------------------------------+\n");
        printf("Choose an option: ");

        if (scanf("%d", &choice) != 1) {
            int ch;

            while ((ch = getchar()) != '\n' && ch != EOF) {
            }

            choice = 0;
        }

        switch (choice) {
            case 1:
                show_dashboard();
                break;

            case 2:
                fiber_trace_dump();
                break;

            case 3:
                fiber_stats_dump();
                break;

            case 4:
                printf("Exiting Scheduler Lab.\n");
                break;

            default:
                printf("Invalid option. Choose 1-4.\n");
                break;
        }
    } while (choice != 4);

    fiber_library_shutdown();

    return EXIT_SUCCESS;
}
