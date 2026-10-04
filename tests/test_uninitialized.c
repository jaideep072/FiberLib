#include "../include/fiber.h"
#include <stdio.h>

static void worker(void *arg)
{
    (void)arg;
}

int main(void)
{
    printf("[Test] Creating fiber before library initialization...\n");

    fiber_id_t id = fiber_create(worker, NULL);

    if (id == 0) {
        printf("[Test] PASS: Creation rejected before initialization.\n");
    } else {
        printf("[Test] FAIL: Fiber created before initialization.\n");
    }

    return 0;
}
