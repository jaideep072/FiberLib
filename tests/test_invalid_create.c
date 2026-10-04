#include "../include/fiber.h"
#include <stdio.h>

int main(void)
{
    if (fiber_library_init() != 0) {
        return 1;
    }

    printf("[Test] Creating fiber with NULL function...\n");

    fiber_id_t id = fiber_create(NULL, NULL);

    if (id == 0) {
        printf("[Test] PASS: NULL fiber function rejected.\n");
    } else {
        printf("[Test] FAIL: NULL fiber function accepted.\n");
    }

    fiber_library_shutdown();

    return 0;
}
