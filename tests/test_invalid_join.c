#include "../include/fiber.h"
#include <stdio.h>

static void worker(void *arg)
{
    (void)arg;

    printf("[Test] Attempting to join invalid fiber...\n");

    int result = fiber_join(9999);

    if (result == -1) {
        printf("[Test] PASS: Invalid fiber ID rejected.\n");
    } else {
        printf("[Test] FAIL: Invalid fiber ID accepted.\n");
    }
}

int main(void)
{
    if (fiber_library_init() != 0) {
        return 1;
    }

    fiber_id_t id = fiber_create(worker, NULL);

    if (id == 0) {
        fiber_library_shutdown();
        return 1;
    }

    fiber_schedule();

    fiber_library_shutdown();

    return 0;
}
