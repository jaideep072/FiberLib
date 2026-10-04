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

    fiber_id_t low = fiber_create(worker, "LOW");
    fiber_id_t normal = fiber_create(worker, "NORMAL");
    fiber_id_t high = fiber_create(worker, "HIGH");

    if (low == 0 || normal == 0 || high == 0) {
        fiber_library_shutdown();
        return 1;
    }

    fiber_set_priority(low, FIBER_PRIORITY_LOW);
    fiber_set_priority(normal, FIBER_PRIORITY_NORMAL);
    fiber_set_priority(high, FIBER_PRIORITY_HIGH);

    fiber_debug_dump();

    printf("[Main] Starting priority scheduler...\n");

    fiber_schedule();

    printf("[Main] All fibers completed.\n");

    fiber_debug_dump();

    fiber_stats_dump();

    fiber_library_shutdown();

    return 0;
}
