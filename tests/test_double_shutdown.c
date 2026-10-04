#include "../include/fiber.h"
#include <stdio.h>

int main(void)
{
    printf("[Test] Initializing library...\n");

    if (fiber_library_init() != 0) {
        return 1;
    }

    printf("[Test] First shutdown...\n");

    fiber_library_shutdown();

    printf("[Test] Second shutdown...\n");

    fiber_library_shutdown();

    printf("[Test] PASS: Double shutdown handled safely.\n");

    return 0;
}
