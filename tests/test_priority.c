#include "../include/fiber.h"
#include <stdio.h>

static void worker(void *arg)
{
    (void)arg;
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

    printf("[Test] Setting invalid priority...\n");

    int result = fiber_set_priority(
        id,
        (fiber_priority_t)99
    );

    if (result == -1) {
        printf("[Test] PASS: Invalid priority rejected.\n");
    } else {
        printf("[Test] FAIL: Invalid priority accepted.\n");
    }

    fiber_library_shutdown();

    return 0;
}
