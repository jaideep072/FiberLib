#include "../include/fiber.h"
#include <stdio.h>

static fiber_id_t self_id;

static void self_join_worker(void *arg)
{
    (void)arg;

    printf("[Test] Fiber attempting to join itself...\n");

    int result = fiber_join(self_id);

    if (result == -1) {
        printf("[Test] PASS: Self-join rejected.\n");
    } else {
        printf("[Test] FAIL: Self-join accepted.\n");
    }
}

int main(void)
{
    if (fiber_library_init() != 0) {
        return 1;
    }

    self_id = fiber_create(self_join_worker, NULL);

    if (self_id == 0) {
        fiber_library_shutdown();
        return 1;
    }

    fiber_schedule();

    fiber_library_shutdown();

    return 0;
}
