#include <stdio.h>
#include "fiber.h"

static int execution_order[9];
static int execution_count = 0;

static void worker(void *arg)
{
int worker_id = *(int *)arg;
int run;

for (run = 0; run < 3; run++) {
    execution_order[execution_count++] = worker_id;
    printf("[Worker %d] Run %d\n", worker_id, run + 1);

    if (run < 2) {
        fiber_yield();
    }
}

}

int main(void)
{
int ids[3] = {1, 2, 3};
int i;
int expected[9] = {1, 2, 3, 1, 2, 3, 1, 2, 3};

if (fiber_library_init() != 0) {
    printf("FAIL: Library initialization failed.\n");
    return 1;
}

if (fiber_set_scheduler_policy(FIBER_SCHEDULER_ROUND_ROBIN) != 0) {
    printf("FAIL: Could not select Round-Robin.\n");
    fiber_library_shutdown();
    return 1;
}

if (fiber_get_scheduler_policy() != FIBER_SCHEDULER_ROUND_ROBIN) {
    printf("FAIL: Scheduler policy was not saved.\n");
    fiber_library_shutdown();
    return 1;
}

if (fiber_create(worker, &ids[0]) == 0 ||
    fiber_create(worker, &ids[1]) == 0 ||
    fiber_create(worker, &ids[2]) == 0) {
    printf("FAIL: Could not create all workers.\n");
    fiber_library_shutdown();
    return 1;
}

/* Deliberately give each fiber a different priority. */
fiber_set_priority(1, FIBER_PRIORITY_HIGH);
fiber_set_priority(2, FIBER_PRIORITY_LOW);
fiber_set_priority(3, FIBER_PRIORITY_NORMAL);

fiber_schedule();

if (execution_count != 9) {
    printf("FAIL: Expected 9 worker runs, got %d.\n",
           execution_count);
    fiber_library_shutdown();
    return 1;
}

for (i = 0; i < 9; i++) {
    if (execution_order[i] != expected[i]) {
        printf("FAIL: Incorrect order at position %d: "
               "expected %d, got %d.\n",
               i, expected[i], execution_order[i]);
        fiber_library_shutdown();
        return 1;
    }
}

printf("PASS: Round-Robin execution order verified.\n");
fiber_library_shutdown();
return 0;

}
