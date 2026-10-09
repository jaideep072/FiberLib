#include "../include/fiber.h"

#include <stdio.h>

static int failures = 0;

static void check(int condition, const char *message)
{
    if (condition) {
        printf("[Test] PASS: %s\n", message);
    } else {
        printf("[Test] FAIL: %s\n", message);
        failures++;
    }
}

static void worker(void *arg)
{
    (void)arg;

    fiber_yield();
    fiber_yield();
}

int main(void)
{
    fiber_id_t id;
    fiber_snapshot_t snapshot;

    printf("[Test] Initializing FiberLib...\n");

    if (fiber_library_init() != 0) {
        printf("[Test] FAIL: Initialization failed.\n");
        return 1;
    }

    id = fiber_create(worker, NULL);
    check(id != 0, "Fiber created successfully");

    if (id == 0) {
        fiber_library_shutdown();
        return 1;
    }

    check(
        fiber_get_snapshot(id, &snapshot) == 0,
        "Snapshot available before scheduling"
    );

    check(
        snapshot.id == id && snapshot.dispatches == 0 &&
        snapshot.yields == 0,
        "Initial snapshot counters are zero"
    );

    fiber_schedule();

    check(
        fiber_get_snapshot(id, &snapshot) == 0,
        "Snapshot available after fiber completion"
    );

    check(
        snapshot.state == FIBER_ZOMBIE,
        "Completed fiber is reported as ZOMBIE"
    );

    check(
        snapshot.dispatches == 3,
        "Dispatch counter records three dispatches"
    );

    check(
        snapshot.yields == 2,
        "Yield counter records two yields"
    );

    check(
        fiber_get_snapshot(id, NULL) == -1,
        "NULL snapshot pointer is rejected"
    );

    check(
        fiber_get_snapshot(9999, &snapshot) == -1,
        "Invalid fiber ID is rejected"
    );

    fiber_library_shutdown();

    if (failures != 0) {
        printf("[Test] %d assertion(s) failed.\n", failures);
        return 1;
    }

    printf("[Test] All snapshot checks passed.\n");
    return 0;
}
