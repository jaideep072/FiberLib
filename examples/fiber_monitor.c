#include "../include/fiber.h"
#include <stdio.h>

/*
 * FiberMonitor
 *
 * A small application-style demonstration of FiberLib.
 *
 * The application contains three cooperative tasks:
 *
 * 1. Network Monitor  - HIGH priority
 * 2. Log Collector    - NORMAL priority
 * 3. Report Generator - LOW priority
 *
 * The Report Generator waits for the Network Monitor
 * using fiber_join() before producing its final report.
 */

static fiber_id_t network_monitor_id;

/*
 * Simulated network monitoring task.
 */
static void network_monitor(void *arg)
{
    (void)arg;

    printf("[Network Monitor] Starting network scan...\n");

    for (int i = 1; i <= 4; i++) {
        printf(
            "[Network Monitor] Scanning network segment %d/4\n",
            i
        );

        fiber_yield();
    }

    printf("[Network Monitor] Network scan completed.\n");
}

/*
 * Simulated log collection task.
 */
static void log_collector(void *arg)
{
    (void)arg;

    printf("[Log Collector] Starting log collection...\n");

    for (int i = 1; i <= 3; i++) {
        printf(
            "[Log Collector] Processing log batch %d/3\n",
            i
        );

        fiber_yield();
    }

    printf("[Log Collector] Log collection completed.\n");
}

/*
 * Report generation depends on the network scan.
 */
static void report_generator(void *arg)
{
    (void)arg;

    printf("[Report Generator] Waiting for network scan...\n");

    if (fiber_join(network_monitor_id) != 0) {
        printf(
            "[Report Generator] Failed to wait for network monitor.\n"
        );

        return;
    }

    printf(
        "[Report Generator] Network scan completed. "
        "Generating final report...\n"
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

int main(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("          FiberMonitor Application\n");
    printf("       Powered by FiberLib Scheduler\n");
    printf("===============================================\n\n");

    /*
     * Initialize FiberLib.
     */
    if (fiber_library_init() != 0) {
        printf("[Application] Failed to initialize FiberLib.\n");
        return 1;
    }

    /*
     * Create application tasks.
     */
    network_monitor_id =
        fiber_create(network_monitor, NULL);

    fiber_id_t log_collector_id =
        fiber_create(log_collector, NULL);

    fiber_id_t report_generator_id =
        fiber_create(report_generator, NULL);

    if (network_monitor_id == 0 ||
        log_collector_id == 0 ||
        report_generator_id == 0) {

        printf("[Application] Failed to create application fibers.\n");

        fiber_library_shutdown();
        return 1;
    }

    /*
     * Assign priorities according to application importance.
     */
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
        FIBER_PRIORITY_LOW
    );

    printf("\n[Application] Application fibers created.\n");
    printf("[Application] Starting scheduler...\n\n");

    /*
     * Show the initial application state.
     */
    fiber_debug_dump();

    /*
     * Start cooperative scheduling.
     */
    fiber_schedule();

    /*
     * Display final runtime information.
     */
    printf("\n[Application] All application tasks completed.\n");

    fiber_stats_dump();

    fiber_library_shutdown();

    printf("\n");
    printf("===============================================\n");
    printf("       FiberMonitor Application Finished\n");
    printf("===============================================\n\n");

    return 0;
}
