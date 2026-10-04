#include "../include/fiber.h"
#include <stdio.h>
#include <string.h>

/*
 * FiberLib Interactive Application
 *
 * Demonstrates:
 * - User-level cooperative fibers
 * - Cooperative Round-Robin scheduling
 * - Priority scheduling
 * - Fiber joining
 * - Multiple fiber creation
 * - Scheduling demonstration
 * - Runtime statistics
 */

/* ---------------------------------------------------------
 * Application Fiber IDs
 * --------------------------------------------------------- */

static fiber_id_t network_monitor_id;

/* ---------------------------------------------------------
 * Custom Fiber Configuration
 * --------------------------------------------------------- */

typedef struct {
    char name[64];
    int steps;
    int verbose;
} custom_fiber_config_t;

/* ---------------------------------------------------------
 * FiberLib Application Demo
 * --------------------------------------------------------- */

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
            "[Report Generator] Failed to wait for network monitor.\n"
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

static void run_application(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("            FiberLib Application\n");
    printf("===============================================\n");

    if (fiber_library_init() != 0) {
        printf(
            "[Application] Failed to initialize FiberLib.\n"
        );

        return;
    }

    network_monitor_id =
        fiber_create(network_monitor, NULL);

    fiber_id_t log_collector_id =
        fiber_create(log_collector, NULL);

    fiber_id_t report_generator_id =
        fiber_create(report_generator, NULL);

    if (network_monitor_id == 0 ||
        log_collector_id == 0 ||
        report_generator_id == 0) {

        printf(
            "[Application] Failed to create application fibers.\n"
        );

        fiber_library_shutdown();
        return;
    }

    fiber_set_priority(
        network_monitor_id,
        FIBER_PRIORITY_HIGH
    );

    fiber_set_priority(
        log_collector_id,
        FIBER_PRIORITY_NORMAL
    );

    fiber_set_priority(
        report_generator_id,
        FIBER_PRIORITY_HIGH
    );

    printf("\n[Application] Application fibers created.\n");

    fiber_debug_dump();

    printf("\n[Application] Starting scheduler...\n\n");

    fiber_schedule();

    printf(
        "\n[Application] All application tasks completed.\n"
    );

    fiber_stats_dump();

    fiber_library_shutdown();
}

/* ---------------------------------------------------------
 * Custom Fiber
 * --------------------------------------------------------- */

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

/* ---------------------------------------------------------
 * Read Custom Fiber Configuration
 * --------------------------------------------------------- */

static int read_custom_fiber_config(
    custom_fiber_config_t *config,
    int *priority_choice)
{
    int verbose_choice;

    memset(config, 0, sizeof(*config));

    printf("Enter fiber name: ");

    if (scanf("%63s", config->name) != 1) {
        printf("[Application] Invalid fiber name.\n");
        return -1;
    }

    printf("Enter number of work steps (1-10): ");

    if (scanf("%d", &config->steps) != 1 ||
        config->steps < 1 ||
        config->steps > 10) {

        printf(
            "[Application] Invalid number of work steps.\n"
        );

        return -1;
    }

    printf("\nSelect priority:\n");
    printf("1. LOW\n");
    printf("2. NORMAL\n");
    printf("3. HIGH\n");
    printf("Enter priority: ");

    if (scanf("%d", priority_choice) != 1 ||
        *priority_choice < 1 ||
        *priority_choice > 3) {

        printf("[Application] Invalid priority.\n");
        return -1;
    }

    printf("\nEnable verbose output?\n");
    printf("1. Yes\n");
    printf("2. No\n");
    printf("Enter choice: ");

    if (scanf("%d", &verbose_choice) != 1 ||
        verbose_choice < 1 ||
        verbose_choice > 2) {

        printf("[Application] Invalid verbose option.\n");
        return -1;
    }

    config->verbose = (verbose_choice == 1);

    return 0;
}

/* ---------------------------------------------------------
 * Multiple Custom Fiber Creation
 * --------------------------------------------------------- */

static void create_custom_fiber(void)
{
    custom_fiber_config_t configs[10];
    fiber_id_t fiber_ids[10];

    int priorities[10];
    int fiber_count;

    printf("\n");
    printf("===============================================\n");
    printf("          Create Multiple Custom Fibers\n");
    printf("===============================================\n");

    printf("How many fibers do you want to create? (1-10): ");

    if (scanf("%d", &fiber_count) != 1 ||
        fiber_count < 1 ||
        fiber_count > 10) {

        printf(
            "[Application] Invalid number of fibers.\n"
        );

        return;
    }

    if (fiber_library_init() != 0) {
        printf(
            "[Application] Failed to initialize FiberLib.\n"
        );

        return;
    }

    for (int i = 0; i < fiber_count; i++) {

        printf("\n");
        printf("-----------------------------------------------\n");
        printf("Configure Fiber %d/%d\n", i + 1, fiber_count);
        printf("-----------------------------------------------\n");

        if (read_custom_fiber_config(
                &configs[i],
                &priorities[i]) != 0) {

            printf(
                "[Application] Fiber configuration failed.\n"
            );

            fiber_library_shutdown();
            return;
        }

        fiber_ids[i] =
            fiber_create(
                custom_worker,
                &configs[i]
            );

        if (fiber_ids[i] == 0) {
            printf(
                "[Application] Failed to create fiber %d.\n",
                i + 1
            );

            fiber_library_shutdown();
            return;
        }

        fiber_set_priority(
            fiber_ids[i],
            (fiber_priority_t)priorities[i]
        );

        printf(
            "\n[Application] Fiber %d created successfully.\n",
            fiber_ids[i]
        );
    }

    printf("\n");
    printf("===============================================\n");
    printf("        Custom Fiber Configuration\n");
    printf("===============================================\n");

    for (int i = 0; i < fiber_count; i++) {
        const char *priority_name;

        if (priorities[i] == 1) {
            priority_name = "LOW";
        }
        else if (priorities[i] == 2) {
            priority_name = "NORMAL";
        }
        else {
            priority_name = "HIGH";
        }

        printf(
            "Fiber %u | Name: %-15s | Steps: %2d | "
            "Priority: %-6s | Verbose: %s\n",
            fiber_ids[i],
            configs[i].name,
            configs[i].steps,
            priority_name,
            configs[i].verbose ? "ON" : "OFF"
        );
    }

    printf("===============================================\n");

    fiber_debug_dump();

    printf("\n[Application] Starting scheduler...\n\n");

    fiber_schedule();

    printf(
        "\n[Application] All custom fibers completed.\n"
    );

    fiber_stats_dump();

    fiber_library_shutdown();
}

/* ---------------------------------------------------------
 * Scheduling Demo Configuration
 * --------------------------------------------------------- */

typedef struct {
    char name[32];
    int rounds;
} scheduling_demo_config_t;

/* ---------------------------------------------------------
 * Scheduling Demo Worker
 * --------------------------------------------------------- */

static void scheduling_demo_worker(void *arg)
{
    scheduling_demo_config_t *config =
        (scheduling_demo_config_t *)arg;

    printf(
        "\n[Scheduling Demo] Fiber %s started.\n",
        config->name
    );

    for (int i = 1; i <= config->rounds; i++) {

        printf(
            "[Scheduling Demo] Fiber %s -> Round %d/%d\n",
            config->name,
            i,
            config->rounds
        );

        fiber_yield();
    }

    printf(
        "[Scheduling Demo] Fiber %s completed.\n",
        config->name
    );
}

/* ---------------------------------------------------------
 * Scheduling Demo
 * --------------------------------------------------------- */

static void scheduling_demo(void)
{
    scheduling_demo_config_t configs[3];
    fiber_id_t fiber_ids[3];

    printf("\n");
    printf("===============================================\n");
    printf("             Scheduling Demo\n");
    printf("===============================================\n");

    printf(
        "[Demo] This demonstration shows cooperative scheduling.\n"
    );

    printf(
        "[Demo] Each fiber performs work and voluntarily yields.\n"
    );

    printf(
        "[Demo] Watch how execution moves between fibers.\n\n"
    );

    if (fiber_library_init() != 0) {
        printf(
            "[Demo] Failed to initialize FiberLib.\n"
        );

        return;
    }

    strcpy(configs[0].name, "A");
    configs[0].rounds = 4;

    strcpy(configs[1].name, "B");
    configs[1].rounds = 4;

    strcpy(configs[2].name, "C");
    configs[2].rounds = 4;

    for (int i = 0; i < 3; i++) {

        fiber_ids[i] =
            fiber_create(
                scheduling_demo_worker,
                &configs[i]
            );

        if (fiber_ids[i] == 0) {
            printf(
                "[Demo] Failed to create scheduling fiber %d.\n",
                i + 1
            );

            fiber_library_shutdown();
            return;
        }

        fiber_set_priority(
            fiber_ids[i],
            FIBER_PRIORITY_NORMAL
        );
    }

    printf(
        "[Demo] Created three NORMAL-priority fibers.\n"
    );

    fiber_debug_dump();

    printf("\n[Demo] Starting scheduler...\n\n");

    fiber_schedule();

    printf(
        "\n[Demo] Scheduling demonstration completed.\n"
    );

    fiber_stats_dump();

    fiber_library_shutdown();
}

/* ---------------------------------------------------------
 * Synchronization Demo
 * --------------------------------------------------------- */

static void synchronization_demo(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("           Synchronization Demo\n");
    printf("===============================================\n");

    printf(
        "[Demo] Synchronization demonstration will be implemented next.\n"
    );
}

/* ---------------------------------------------------------
 * Fiber Information
 * --------------------------------------------------------- */

static void fiber_information(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("             Fiber Information\n");
    printf("===============================================\n");

    printf(
        "[Info] Fiber information menu will be implemented next.\n"
    );
}

/* ---------------------------------------------------------
 * Library Features
 * --------------------------------------------------------- */

static void show_features(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("            FiberLib Features\n");
    printf("===============================================\n");

    printf("1. User-level cooperative fibers\n");
    printf("2. Manual stack allocation\n");
    printf("3. POSIX ucontext-based switching\n");
    printf("4. Priority-aware scheduling\n");
    printf("5. Priority aging for starvation prevention\n");
    printf("6. Fiber join and blocking\n");
    printf("7. Circular wait deadlock detection\n");
    printf("8. Fiber state tracking\n");
    printf("9. Runtime scheduling statistics\n");
    printf("10. Debug state inspection\n");
    printf("11. Multiple fiber creation\n");
    printf("12. Interactive scheduling demonstration\n");
    printf("13. Interactive application interface\n");
}

/* ---------------------------------------------------------
 * Main Menu
 * --------------------------------------------------------- */

static void show_menu(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("                 FiberLib\n");
    printf("          User-Level Thread Library\n");
    printf("===============================================\n");
    printf("1. Run FiberLib Application\n");
    printf("2. Create Custom Fiber(s)\n");
    printf("3. Run Scheduling Demo\n");
    printf("4. Run Synchronization Demo\n");
    printf("5. View Fiber Information\n");
    printf("6. Show Library Features\n");
    printf("7. Exit\n");
    printf("===============================================\n");
    printf("Enter your choice: ");
}

/* ---------------------------------------------------------
 * Main
 * --------------------------------------------------------- */

int main(void)
{
    int choice;

    while (1) {

        show_menu();

        if (scanf("%d", &choice) != 1) {
            printf(
                "\n[Application] Invalid input. "
                "Please enter a number.\n"
            );

            return 1;
        }

        switch (choice) {

            case 1:
                run_application();
                break;

            case 2:
                create_custom_fiber();
                break;

            case 3:
                scheduling_demo();
                break;

            case 4:
                synchronization_demo();
                break;

            case 5:
                fiber_information();
                break;

            case 6:
                show_features();
                break;

            case 7:
                printf(
                    "\n[Application] Exiting FiberLib.\n"
                );

                return 0;

            default:
                printf(
                    "\n[Application] Invalid choice. "
                    "Please select 1-7.\n"
                );
        }
    }
}
