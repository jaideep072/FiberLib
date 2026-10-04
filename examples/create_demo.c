#include "../include/fiber.h"
#include <stdio.h>

static void worker(void *arg)
{
    const char *message = (const char *)arg;

    printf("[Fiber] %s\n", message);
}

int main(void)
{
    if (fiber_library_init() != 0) {
        return 1;
    }

    fiber_id_t fiber1 = fiber_create(
        worker,
        "Hello from Fiber 1"
    );

    if (fiber1 == 0) {
        fiber_library_shutdown();
        return 1;
    }

    fiber_debug_dump();

    printf("[Main] Starting scheduler...\n");

    fiber_schedule();

    printf("[Main] Scheduler returned.\n");

    fiber_debug_dump();

    fiber_library_shutdown();

    return 0;
}
