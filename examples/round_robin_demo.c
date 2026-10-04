#include "../include/fiber.h"
#include <stdio.h>

static void worker(void *arg)
{
    const char *name = (const char *)arg;

    for (int i = 1; i <= 3; i++) {
        printf("[Fiber %s] Step %d\n", name, i);

        fiber_yield();
    }

    printf("[Fiber %s] Finished.\n", name);
}

int main(void)
{
    if (fiber_library_init() != 0) {
        return 1;
    }

    fiber_create(worker, "A");
    fiber_create(worker, "B");
    fiber_create(worker, "C");

    fiber_debug_dump();

    printf("[Main] Starting Round-Robin scheduler...\n");

    fiber_schedule();

    printf("[Main] All fibers completed.\n");

    fiber_debug_dump();

    fiber_stats_dump();

    fiber_library_shutdown();

    return 0;
}
