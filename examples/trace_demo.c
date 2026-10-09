#include "../include/fiber.h"
#include <stdio.h>

static void worker_a(void *arg)
{
    (void)arg;

    printf("[Worker A] Running...\n");
    fiber_yield();

    printf("[Worker A] Running again...\n");
    fiber_yield();

    printf("[Worker A] Finished.\n");
}

static void worker_b(void *arg)
{
    (void)arg;

    printf("[Worker B] Running...\n");
    fiber_yield();

    printf("[Worker B] Running again...\n");
    fiber_yield();

    printf("[Worker B] Finished.\n");
}

static void worker_c(void *arg)
{
    (void)arg;

    printf("[Worker C] Running...\n");
    fiber_yield();

    printf("[Worker C] Finished.\n");
}

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("       FiberLib Scheduler Trace Demo\n");
    printf("========================================\n\n");

    if (fiber_library_init() != 0) {
        printf("[Demo] Failed to initialize FiberLib.\n");
        return 1;
    }

    fiber_create(worker_a, NULL);
    fiber_create(worker_b, NULL);
    fiber_create(worker_c, NULL);

    printf("\n[Demo] Starting scheduler...\n\n");

    fiber_schedule();

    printf("\n[Demo] Scheduler finished.\n");

    printf("\n========================================\n");
    printf("             EVENT TRACE\n");
    printf("========================================\n");

    printf("Recorded events: %lu\n\n", fiber_trace_count());

    fiber_trace_dump();

    fiber_library_shutdown();

    return 0;
}
