#include "include/fiber.h"
#include <stdio.h>
#include <string.h>

static fiber_id_t network_monitor_id;

typedef struct {
    char name[64];
    int steps;
    int verbose;
} custom_fiber_config_t;

static void network_monitor(void *arg)
{
    (void)arg;

    printf("\n[Network Monitor] Starting network monitoring...\n");

    for (int i = 1; i <= 4; i++) {
        printf(
            "[Network Monitor] Scanning network segment %d/4\n",
            i
        );

        fiber_yield();
    }

    printf("[Network Monitor] Monitoring completed.\n");
}

static void log_collector(void *arg)
{
    (void)arg;

    printf("\n[Log Collector] Starting log collection...\n");

    for (int i = 1; i <= 4; i++) {
        printf(
            "[Log Collector] Processing log batch %d/4\n",
            i
        );

        fiber_yield();
    }

    printf("[Log Collector] Log collection completed.\n");
}

static void report_generator(void *arg)
{
    (void)arg;

    printf("\n[Report Generator] Waiting for network monitor...\n");

    if (fiber_join(network_monitor_id) != 0) {
        printf(
            "[Report Generator] Failed to join network monitor.\n"
        );

        return;
    }

    printf(
        "[Report Generator] Network monitoring completed.\n"
    );

    printf(
        "[Report Generator] Generating final report...\n"
    );

    for (int i = 1; i <= 2; i++) {
        printf(
            "[Report Generator] Generating report section %d/2\n",
            i
        );

        fiber_yield();
    }

    printf("[Report Generator] Final report generated.\n");
}

static void custom_worker(void *arg)
{
    custom_fiber_config_t *config =
        (custom_fiber_config_t *)arg;

    printf(
        "\n[Custom Fiber: %s] Started.\n",
        config->name
    );

    printf(
        "[Custom Fiber: %s] Total work steps: %d\n",
        config->name,
        config->steps
    );

    if (config->verbose) {
        printf(
            "[Custom Fiber: %s] Verbose mode enabled.\n",
            config->name
        );
    }

    for (int i = 1; i <= config->steps; i++) {

        if (config->verbose) {
            printf(
                "[Custom Fiber: %s] Executing work step %d/%d\n",
                config->name,
                i,
                config->steps
            );
        }
        else {
            printf(
                "[Custom Fiber: %s] Step %d/%d\n",
                config->name,
                i,
                config->steps
            );
        }

        fiber_yield();
    }

    printf(
        "[Custom Fiber: %s] Work completed.\n",
        config->name
    );
}

static void run_application(void)
{
    fiber_id_t log_collector_id;
    fiber_id_t report_generator_id;

    printf("\n");
    printf("===============================================\n");
    printf("              FiberLib Application\n");
    printf("       User-Level Thread Demonstration\n");
    printf("===============================================\n");

    printf("\n[Application] Initializing FiberLib...\n");

    if (fiber_library_init() != 0) {
        printf("[Application] Initialization failed.\n");
        return;
    }

    network_monitor_id =
        fiber_create(network_monitor, NULL);

    log_collector_id =
        fiber_create(log_collector, NULL);

    report_generator_id =
        fiber_create(report_generator, NULL);

    if (network_monitor_id == 0 ||
        log_collector_id == 0 ||
        report_generator_id == 0) {

        printf("[Application] Fiber creation failed.\n");

        fiber_library_shutdown();
        return;
    }

    fiber_set_priority(
        network_monitor_id,
        FIBER_PRIORITY_HIGH
    );

    fiber_set_priority(
        report_generator_id,
        FIBER_PRIORITY_HIGH
    );

    fiber_set_priority(
        log_collector_id,
        FIBER_PRIORITY_NORMAL
    );

    printf("\n[Application] Three application tasks created.\n");
    printf("[Application] Priorities assigned:\n");
    printf("  Network Monitor  : HIGH\n");
    printf("  Report Generator : HIGH\n");
    printf("  Log Collector    : NORMAL\n");

    printf("\n[Application] Initial Fiber State:\n");

    fiber_debug_dump();

    printf("\n[Application] Starting user-level scheduler...\n");

    fiber_schedule();

    printf("\n[Application] All tasks completed.\n");

    fiber_stats_dump();

    fiber_library_shutdown();

    printf("\n===============================================\n");
    printf("          FiberLib Application Complete\n");
    printf("===============================================\n");
}

static void create_custom_fiber(void)
{
    custom_fiber_config_t config;
    int priority_choice;
    int verbose_choice;

    memset(&config, 0, sizeof(config));

    printf("\n");
    printf("===============================================\n");
    printf("              Create Custom Fiber\n");
    printf("===============================================\n");

    printf("Enter fiber name: ");

    if (scanf("%63s", config.name) != 1) {
        printf("[Application] Invalid fiber name.\n");

        int ch;

        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* Clear invalid input. */
        }

        return;
    }

    printf("Enter number of work steps (1-10): ");

    if (scanf("%d", &config.steps) != 1) {
        printf("[Application] Invalid input.\n");

        int ch;

        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* Clear invalid input. */
        }

        return;
    }

    if (config.steps < 1 || config.steps > 10) {
        printf("[Application] Steps must be between 1 and 10.\n");
        return;
    }

    printf("\nSelect priority:\n");
    printf("1. LOW\n");
    printf("2. NORMAL\n");
    printf("3. HIGH\n");
    printf("Enter priority: ");

    if (scanf("%d", &priority_choice) != 1) {
        printf("[Application] Invalid input.\n");

        int ch;

        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* Clear invalid input. */
        }

        return;
    }

    if (priority_choice < 1 || priority_choice > 3) {
        printf("[Application] Invalid priority.\n");
        return;
    }

    printf("\nEnable verbose output?\n");
    printf("1. Yes\n");
    printf("2. No\n");
    printf("Enter choice: ");

    if (scanf("%d", &verbose_choice) != 1) {
        printf("[Application] Invalid input.\n");

        int ch;

        while ((ch = getchar()) != '\n' && ch != EOF) {
            /* Clear invalid input. */
        }

        return;
    }

    if (verbose_choice != 1 && verbose_choice != 2) {
        printf("[Application] Invalid verbose option.\n");
        return;
    }

    config.verbose = (verbose_choice == 1);

    printf("\n[Application] Configuration:\n");
    printf("  Fiber Name   : %s\n", config.name);
    printf("  Work Steps   : %d\n", config.steps);

    if (priority_choice == 1) {
        printf("  Priority     : LOW\n");
    }
    else if (priority_choice == 2) {
        printf("  Priority     : NORMAL\n");
    }
    else {
        printf("  Priority     : HIGH\n");
    }

    printf(
        "  Verbose Mode : %s\n",
        config.verbose ? "ON" : "OFF"
    );

    if (fiber_library_init() != 0) {
        printf("[Application] Failed to initialize FiberLib.\n");
        return;
    }

    fiber_id_t fiber_id =
        fiber_create(custom_worker, &config);

    if (fiber_id == 0) {
        printf("[Application] Failed to create custom fiber.\n");

        fiber_library_shutdown();
        return;
    }

    fiber_priority_t priority =
        (fiber_priority_t)priority_choice;

    if (fiber_set_priority(fiber_id, priority) != 0) {
        printf("[Application] Failed to set fiber priority.\n");

        fiber_library_shutdown();
        return;
    }

    printf(
        "\n[Application] Custom fiber %u created successfully.\n",
        fiber_id
    );

    fiber_debug_dump();

    printf("\n[Application] Starting scheduler...\n");

    fiber_schedule();

    printf("\n[Application] Custom fiber execution completed.\n");

    fiber_stats_dump();

    fiber_library_shutdown();
}

static void scheduling_demo(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("              Scheduling Demo\n");
    printf("===============================================\n");

    printf("\n[Application] Scheduling demo will be implemented next.\n");
}

static void synchronization_demo(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("            Synchronization Demo\n");
    printf("===============================================\n");

    printf("\n[Application] Synchronization demo will be implemented next.\n");
}

static void fiber_information(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("              Fiber Information\n");
    printf("===============================================\n");

    printf("\n[Application] Fiber information menu will be implemented next.\n");
}

static void show_features(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("             FiberLib Features\n");
    printf("===============================================\n");

    printf("\n1. User-Level Fibers\n");
    printf("   Fibers execute completely in user space.\n");

    printf("\n2. Cooperative Scheduling\n");
    printf("   Fibers voluntarily yield CPU control.\n");

    printf("\n3. Priority Scheduling\n");
    printf("   LOW, NORMAL and HIGH priorities are supported.\n");

    printf("\n4. Round-Robin Scheduling\n");
    printf("   Ready fibers receive scheduling opportunities.\n");

    printf("\n5. Priority Aging\n");
    printf("   Aging helps prevent starvation of low-priority fibers.\n");

    printf("\n6. Fiber Join\n");
    printf("   A fiber can wait for another fiber to complete.\n");

    printf("\n7. Deadlock Detection\n");
    printf("   Circular fiber dependencies can be detected.\n");

    printf("\n8. Runtime Statistics\n");
    printf("   Dispatches, yields, context switches and completions\n");
    printf("   are tracked by the library.\n");

    printf("\n9. Dynamic Fiber Creation\n");
    printf("   Applications can create fibers using fiber_create().\n");

    printf("\n===============================================\n");
}

static void show_menu(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("                 FiberLib\n");
    printf("          User-Level Thread Library\n");
    printf("===============================================\n");
    printf("1. Run FiberLib Application\n");
    printf("2. Create Custom Fiber\n");
    printf("3. Run Scheduling Demo\n");
    printf("4. Run Synchronization Demo\n");
    printf("5. View Fiber Information\n");
    printf("6. Show Library Features\n");
    printf("7. Exit\n");
    printf("===============================================\n");
}

int main(void)
{
    int choice;

    while (1) {
        show_menu();

        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1) {
            printf("\n[Application] Invalid input.\n");

            int ch;

            while ((ch = getchar()) != '\n' && ch != EOF) {
                /* Clear invalid input. */
            }

            continue;
        }

        if (choice == 1) {
            run_application();
        }
        else if (choice == 2) {
            create_custom_fiber();
        }
        else if (choice == 3) {
            scheduling_demo();
        }
        else if (choice == 4) {
            synchronization_demo();
        }
        else if (choice == 5) {
            fiber_information();
        }
        else if (choice == 6) {
            show_features();
        }
        else if (choice == 7) {
            printf("\n[Application] Exiting FiberLib.\n");
            break;
        }
        else {
            printf("\n[Application] Invalid choice.\n");
        }
    }

    return 0;
}
