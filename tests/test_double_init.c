#include "../include/fiber.h"
#include <stdio.h>

int main(void)
{
    printf("[Test] First initialization...\n");

    int first = fiber_library_init();

    printf("[Test] Second initialization...\n");

    int second = fiber_library_init();

    if (first == 0 && second == 0) {
        printf("[Test] PASS: Double initialization handled safely.\n");
    } else {
        printf("[Test] FAIL: Double initialization failed.\n");
    }

    fiber_library_shutdown();

    return 0;
}
