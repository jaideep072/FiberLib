#include "../include/fiber.h"
#include <stdio.h>

static fiber_id_t worker_id;

static void worker(void *arg)
{
    (void)arg;

    printf("[Worker] Running and finishing immediately.\n");
}

static void joiner(void *arg)
{
    (void)arg;

    printf("[Joiner] Trying to join completed worker...\n");

    int result = fiber_join(worker_id);

    if (result == 0) {
        printf("[Test] PASS: Completed fiber joined successfully.\n");
    } else {
        printf("[Test] FAIL: Joining completed fiber failed.\n");
    }
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

    /*
     * Create the joiner after the worker.
     * The worker will run first and finish.
     */
    fiber_id_t joiner_id = fiber_create(joiner, NULL);

    if (joiner_id == 0) {
        fiber_library_shutdown();
        return 1;
    }

    fiber_schedule();

    fiber_library_shutdown();

    return 0;
}
