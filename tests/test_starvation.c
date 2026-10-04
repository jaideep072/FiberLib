#include "../include/fiber.h"
#include <stdio.h>

static int high_runs = 0;
static int low_runs = 0;

static void high_priority_fiber(void *arg)
{
    (void)arg;

    for (int i = 0; i < 15; i++) {
        high_runs++;

        printf(
            "[HIGH] Run %d\n",
            high_runs
        );

        fiber_yield();
    }
}

static void low_priority_fiber(void *arg)
{
    (void)arg;

    for (int i = 0; i < 5; i++) {
        low_runs++;

        printf(
            "[LOW] Run %d\n",
            low_runs
        );

        fiber_yield();
    }
}

int main(void)
{
    if (fiber_library_init() != 0) {
        return 1;
    }

    fiber_id_t high =
        fiber_create(high_priority_fiber, NULL);

    fiber_id_t low =
        fiber_create(low_priority_fiber, NULL);

    if (high == 0 || low == 0) {
        fiber_library_shutdown();
        return 1;
    }

    fiber_set_priority(
        high,
        FIBER_PRIORITY_HIGH
    );

    fiber_set_priority(
        low,
        FIBER_PRIORITY_LOW
    );

    printf("\n[Test] Starting starvation prevention test...\n\n");

    fiber_schedule();

    printf("\n[Test] High-priority runs: %d\n",
           high_runs);

    printf("[Test] Low-priority runs: %d\n",
           low_runs);

    if (low_runs > 0) {
        printf(
            "[Test] PASS: Low-priority fiber received CPU time.\n"
        );
    } else {
        printf(
            "[Test] FAIL: Low-priority fiber was starved.\n"
        );
    }

    fiber_library_shutdown();

    return 0;
}
