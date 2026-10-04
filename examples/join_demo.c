#include "../include/fiber.h"
#include <stdio.h>

static fiber_id_t worker_id;

static void worker(void *arg)
{
    (void)arg;

    printf("[Worker] Started.\n");

    for (int i = 1; i <= 3; i++) {
        printf("[Worker] Step %d\n", i);
        fiber_yield();
    }

    printf("[Worker] Finished.\n");
}

static void waiter(void *arg)
{
    (void)arg;

    printf("[Waiter] Started.\n");

    printf("[Waiter] Waiting for Worker...\n");

    if (fiber_join(worker_id) == 0) {
        printf("[Waiter] Worker has finished. Continuing...\n");
    }

    printf("[Waiter] Finished.\n");
}

int main(void)
{
    if (fiber_library_init() != 0) {
        return 1;
    }

    worker_id = fiber_create(worker, NULL);

    if (worker_id == 0) {
        fiber_library_shutdown();
        return 1;
    }

    fiber_id_t waiter_id = fiber_create(waiter, NULL);

    if (waiter_id == 0) {
        fiber_library_shutdown();
        return 1;
    }

    fiber_debug_dump();

    printf("[Main] Starting scheduler...\n");

    fiber_schedule();

    printf("[Main] All fibers completed.\n");

    fiber_debug_dump();

    fiber_library_shutdown();

    return 0;
}
