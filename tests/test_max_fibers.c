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

    int successful = 0;

    for (int i = 0; i < 129; i++) {
        fiber_id_t id = fiber_create(worker, NULL);

        if (id != 0) {
            successful++;
        }
    }

    printf("[Test] Successfully created fibers: %d\n", successful);

    if (successful == 128) {
        printf("[Test] PASS: Maximum fiber limit enforced.\n");
    } else {
        printf("[Test] FAIL: Maximum fiber limit not enforced.\n");
    }

    fiber_library_shutdown();

    return 0;
}
