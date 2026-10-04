#include "../include/fiber.h"
#include <stdio.h>

static fiber_id_t fiber_a;
static fiber_id_t fiber_b;

static void worker_a(void *arg)
{
    (void)arg;

    printf("[Fiber A] Waiting for Fiber B...\n");

    fiber_join(fiber_b);

    printf("[Fiber A] Continued.\n");
}

static void worker_b(void *arg)
{
    (void)arg;

    printf("[Fiber B] Waiting for Fiber A...\n");

    fiber_join(fiber_a);

    printf("[Fiber B] Continued.\n");
}

int main(void)
{
    if (fiber_library_init() != 0) {
        return 1;
    }

    fiber_a = fiber_create(worker_a, NULL);
    fiber_b = fiber_create(worker_b, NULL);

    if (fiber_a == 0 || fiber_b == 0) {
        fiber_library_shutdown();
        return 1;
    }

    printf("[Test] Starting circular join test...\n");

    fiber_schedule();

    printf("[Test] Scheduler returned.\n");

    fiber_debug_dump();

    fiber_library_shutdown();

    return 0;
}
