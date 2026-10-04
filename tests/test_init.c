#include "../include/fiber.h"

int main(void)
{
    if (fiber_library_init() != 0) {
        return 1;
    }

    fiber_debug_dump();

    fiber_library_shutdown();

    return 0;
}
