#include "../include/fiber.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================
 * FiberLib Interactive Application
 * ========================================================= */

typedef struct {
    char name[64];
    int steps;
    int verbose;
} custom_fiber_config_t;

typedef struct {
    char name[32];
    int rounds;
    fiber_priority_t priority;
} priority_demo_config_t;

typedef struct {
    char name[32];
    int rounds;
    fiber_priority_t priority;
} aging_demo_config_t;

/* =========================================================
 * Application Fibers
 * ========================================================= */

static fiber_id_t network_monitor_id;

static void network_monitor(void *arg)
{
    (void)arg;

    for (int i = 1; i <= 4; i++) {
        printf(
            "[Network Monitor] Checking network status... %d/4\n",
            i
        );

        fiber_yield();
    }

    printf(
        "[Network Monitor] Monitoring completed.\n"
    );
}

static void log_collector(void *arg)
{
    (void)arg;

    for (int i = 1; i <= 4; i++) {
        printf(
            "[Log Collector] Processing log batch %d/4\n",
            i
        );

        fiber_yield();
    }

    printf(
        "[Log Collector] Log collection completed.\n"
    );
}

static void report_generator(void *arg)
{
    (void)arg;

    printf(
        "[Report Generator] Waiting for Network Monitor...\n"
    );

    if (fiber_join(network_monitor_id) != 0) {
        printf(
            "[Report Generator] Failed to join Network Monitor.\n"
        );

        return;
    }

    printf(
        "[Report Generator] Network Monitor completed.\n"
    );

    for (int i = 1; i <= 2; i++) {
        printf(
            "[Report Generator] Generating report section %d/2\n",
            i
        );

        fiber_yield();
    }

    printf(
        "[Report Generator] Report completed.\n"
    );
}

/* =========================================================
 * Custom Fiber
 * ========================================================= */

static void custom_worker(void *arg)
{
    custom_fiber_config_t *config =
        (custom_fiber_config_t *)arg;

    printf(
        "\n[Custom Fiber: %s] Starting work...\n",
        config->name
    );

    if (config->verbose) {
        printf(
            "[Custom Fiber: %s] Verbose mode enabled.\n",
            config->name
        );
    }

    for (int i = 1; i <= config->steps; i++) {
        printf(
            "[Custom Fiber: %s] Executing work step %d/%d\n",
            config->name,
            i,
            config->steps
        );

        if (config->verbose) {
            printf(
                "[Custom Fiber: %s] Processing data for step %d.\n",
                config->name,
                i
            );
        }

        fiber_yield();
    }

    printf(
        "[Custom Fiber: %s] Work completed.\n",
        config->name
    );
}

/* =========================================================
 * Scheduling Demo
 * ========================================================= */

static void scheduling_worker(void *arg)
{
    char *name = (char *)arg;

    for (int i = 1; i <= 4; i++) {
        printf(
            "[Scheduling Demo] Fiber %s -> Round %d/4\n",
            name,
            i
        );

        fiber_yield();
    }

    printf(
        "[Scheduling Demo] Fiber %s completed.\n",
        name
    );
}

static void scheduling_demo(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("             Scheduling Demo\n");
    printf("===============================================\n");

    printf(
        "[Demo] This demonstrates cooperative Round-Robin scheduling.\n"
    );

    printf(
        "[Demo] Each fiber voluntarily yields after every round.\n\n"
    );

    if (fiber_library_init() != 0) {
        printf(
            "[Demo] Failed to initialize FiberLib.\n"
        );

        return;
    }

    char fiber_a[] = "A";
    char fiber_b[] = "B";
    char fiber_c[] = "C";

    fiber_id_t a =
        fiber_create(scheduling_worker, fiber_a);

    fiber_id_t b =
        fiber_create(scheduling_worker, fiber_b);

    fiber_id_t c =
        fiber_create(scheduling_worker, fiber_c);

    if (a == 0 || b == 0 || c == 0) {
        printf(
            "[Demo] Failed to create scheduling fibers.\n"
        );

        fiber_library_shutdown();

        return;
    }

    printf(
        "[Demo] Created three fibers: A, B and C.\n"
    );

    fiber_debug_dump();

    printf(
        "\n[Demo] Starting Round-Robin scheduler...\n\n"
    );

    fiber_schedule();

    printf(
        "\n[Demo] Scheduling demonstration completed.\n"
    );

    fiber_stats_dump();

    fiber_library_shutdown();

    printf("\n");
}

/* =========================================================
 * Priority Scheduling Demo
 * ========================================================= */

static const char *priority_name(
    fiber_priority_t priority
)
{
    switch (priority) {
        case FIBER_PRIORITY_LOW:
            return "LOW";

        case FIBER_PRIORITY_NORMAL:
            return "NORMAL";

        case FIBER_PRIORITY_HIGH:
            return "HIGH";

        default:
            return "UNKNOWN";
    }
}

static void priority_demo_worker(void *arg)
{
    priority_demo_config_t *config =
        (priority_demo_config_t *)arg;

    printf(
        "\n[Priority Demo] Fiber %s started | Priority: %s\n",
        config->name,
        priority_name(config->priority)
    );

    for (int i = 1; i <= config->rounds; i++) {
        printf(
            "[Priority Demo] Fiber %s | Priority: %s | Round %d/%d\n",
            config->name,
            priority_name(config->priority),
            i,
            config->rounds
        );

        fiber_yield();
    }

    printf(
        "[Priority Demo] Fiber %s completed.\n",
        config->name
    );
}

static void priority_scheduling_demo(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("          Priority Scheduling Demo\n");
    printf("===============================================\n");

    printf(
        "[Demo] Higher-priority fibers receive more\n"
        "       scheduling opportunities.\n"
    );

    printf(
        "[Demo] Priority values: LOW=1, NORMAL=2, HIGH=3.\n\n"
    );

    if (fiber_library_init() != 0) {
        printf(
            "[Demo] Failed to initialize FiberLib.\n"
        );

        return;
    }

    priority_demo_config_t high_config = {
        "HIGH",
        6,
        FIBER_PRIORITY_HIGH
    };

    priority_demo_config_t normal_config = {
        "NORMAL",
        6,
        FIBER_PRIORITY_NORMAL
    };

    priority_demo_config_t low_config = {
        "LOW",
        6,
        FIBER_PRIORITY_LOW
    };

    fiber_id_t high =
        fiber_create(priority_demo_worker, &high_config);

    fiber_id_t normal =
        fiber_create(priority_demo_worker, &normal_config);

    fiber_id_t low =
        fiber_create(priority_demo_worker, &low_config);

    if (high == 0 || normal == 0 || low == 0) {
        printf(
            "[Demo] Failed to create priority fibers.\n"
        );

        fiber_library_shutdown();

        return;
    }

    fiber_set_priority(
        high,
        FIBER_PRIORITY_HIGH
    );

    fiber_set_priority(
        normal,
        FIBER_PRIORITY_NORMAL
    );

    fiber_set_priority(
        low,
        FIBER_PRIORITY_LOW
    );

    printf(
        "[Demo] Created HIGH, NORMAL and LOW priority fibers.\n"
    );

    fiber_debug_dump();

    printf(
        "\n[Demo] Starting priority-aware scheduler...\n\n"
    );

    fiber_schedule();

    printf(
        "\n[Demo] Priority scheduling demonstration completed.\n"
    );

    fiber_stats_dump();

    fiber_library_shutdown();

    printf("\n");
}

/* =========================================================
 * Priority Aging Demo
 * ========================================================= */

static void aging_demo_worker(void *arg)
{
    aging_demo_config_t *config =
        (aging_demo_config_t *)arg;

    printf(
        "\n[Aging Demo] Fiber %s started | Priority: %s\n",
        config->name,
        priority_name(config->priority)
    );

    for (int i = 1; i <= config->rounds; i++) {
        printf(
            "[Aging Demo] Fiber %s | Priority: %s | Round %d/%d\n",
            config->name,
            priority_name(config->priority),
            i,
            config->rounds
        );

        fiber_yield();
    }

    printf(
        "[Aging Demo] Fiber %s completed.\n",
        config->name
    );
}

static void priority_aging_demo(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("             Priority Aging Demo\n");
    printf("===============================================\n");

    printf(
        "[Demo] Aging prevents low-priority fibers\n"
        "       from being permanently starved.\n"
    );

    printf(
        "[Demo] Waiting fibers accumulate aging points.\n"
    );

    printf(
        "[Demo] Once aging reaches the scheduler threshold,\n"
        "       the effective priority of the fiber increases.\n\n"
    );

    if (fiber_library_init() != 0) {
        printf(
            "[Demo] Failed to initialize FiberLib.\n"
        );

        return;
    }

    aging_demo_config_t high_config = {
        "HIGH",
        12,
        FIBER_PRIORITY_HIGH
    };

    aging_demo_config_t normal_config = {
        "NORMAL",
        8,
        FIBER_PRIORITY_NORMAL
    };

    aging_demo_config_t low_config = {
        "LOW",
        6,
        FIBER_PRIORITY_LOW
    };

    fiber_id_t high =
        fiber_create(aging_demo_worker, &high_config);

    fiber_id_t normal =
        fiber_create(aging_demo_worker, &normal_config);

    fiber_id_t low =
        fiber_create(aging_demo_worker, &low_config);

    if (high == 0 || normal == 0 || low == 0) {
        printf(
            "[Demo] Failed to create aging demonstration fibers.\n"
        );

        fiber_library_shutdown();

        return;
    }

    fiber_set_priority(
        high,
        FIBER_PRIORITY_HIGH
    );

    fiber_set_priority(
        normal,
        FIBER_PRIORITY_NORMAL
    );

    fiber_set_priority(
        low,
        FIBER_PRIORITY_LOW
    );

    printf(
        "[Demo] Created HIGH, NORMAL and LOW priority fibers.\n"
    );

    printf(
        "[Demo] The LOW priority fiber will demonstrate aging.\n\n"
    );

    fiber_debug_dump();

    printf(
        "\n[Demo] Starting scheduler with priority aging...\n\n"
    );

    fiber_schedule();

    printf(
        "\n[Demo] Priority aging demonstration completed.\n"
    );

    printf(
        "[Demo] Final fiber state:\n\n"
    );

    fiber_debug_dump();

    fiber_stats_dump();

    fiber_library_shutdown();

    printf("\n");
}

/* =========================================================
 * Main FiberLib Application
 * ========================================================= */

static void run_application(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("          Running FiberLib Application\n");
    printf("===============================================\n");

    if (fiber_library_init() != 0) {
        printf(
            "[Application] Failed to initialize FiberLib.\n"
        );

        return;
    }

    fiber_id_t network_monitor_fiber =
        fiber_create(network_monitor, NULL);

    network_monitor_id =
        network_monitor_fiber;

    fiber_id_t logger =
        fiber_create(log_collector, NULL);

    fiber_id_t report =
        fiber_create(report_generator, NULL);

    if (network_monitor_fiber == 0 ||
        logger == 0 ||
        report == 0) {

        printf(
            "[Application] Failed to create application fibers.\n"
        );

        fiber_library_shutdown();

        return;
    }

    fiber_set_priority(
        network_monitor_fiber,
        FIBER_PRIORITY_HIGH
    );

    fiber_set_priority(
        report,
        FIBER_PRIORITY_HIGH
    );

    fiber_set_priority(
        logger,
        FIBER_PRIORITY_NORMAL
    );

    printf(
        "\n[Application] Created application fibers.\n"
    );

    fiber_debug_dump();

    printf(
        "\n[Application] Starting scheduler...\n\n"
    );

    fiber_schedule();

    printf(
        "\n[Application] FiberLib application completed.\n"
    );

    fiber_stats_dump();

    fiber_library_shutdown();

    printf("\n");
}

/* =========================================================
 * Custom Fiber Creation
 * ========================================================= */

static void create_custom_fiber(void)
{
    int count;

    printf("\n");
    printf("===============================================\n");
    printf("             Create Custom Fiber(s)\n");
    printf("===============================================\n");

    printf(
        "How many fibers do you want to create? (1-10): "
    );

    if (scanf("%d", &count) != 1) {
        printf(
            "[Application] Invalid input.\n"
        );

        while (getchar() != '\n') {
        }

        return;
    }

    if (count < 1 || count > 10) {
        printf(
            "[Application] Please enter a number between 1 and 10.\n"
        );

        return;
    }

    custom_fiber_config_t configs[10];
    fiber_id_t ids[10];

    for (int i = 0; i < count; i++) {
        printf(
            "\n--- Configuration for Fiber %d ---\n",
            i + 1
        );

        printf(
            "Fiber name: "
        );

        scanf(
            "%63s",
            configs[i].name
        );

        printf(
            "Number of work steps: "
        );

        if (scanf(
                "%d",
                &configs[i].steps
            ) != 1) {

            printf(
                "[Application] Invalid step count.\n"
            );

            while (getchar() != '\n') {
            }

            return;
        }

        if (configs[i].steps < 1) {
            configs[i].steps = 1;
        }

        int priority;

        printf(
            "Priority (1=LOW, 2=NORMAL, 3=HIGH): "
        );

        if (scanf(
                "%d",
                &priority
            ) != 1) {

            printf(
                "[Application] Invalid priority.\n"
            );

            while (getchar() != '\n') {
            }

            return;
        }

        if (priority < 1 || priority > 3) {
            printf(
                "[Application] Invalid priority. Using NORMAL.\n"
            );

            priority = 2;
        }

        configs[i].verbose = 0;

        char verbose_choice;

        printf(
            "Enable verbose mode? (y/n): "
        );

        scanf(
            " %c",
            &verbose_choice
        );

        if (verbose_choice == 'y' ||
            verbose_choice == 'Y') {

            configs[i].verbose = 1;
        }

        printf(
            "\n[Application] Configuration:\n"
        );

        printf(
            "  Fiber Name   : %s\n",
            configs[i].name
        );

        printf(
            "  Work Steps   : %d\n",
            configs[i].steps
        );

        printf(
            "  Priority     : %s\n",
            priority_name(
                (fiber_priority_t)priority
            )
        );

        printf(
            "  Verbose Mode : %s\n",
            configs[i].verbose
                ? "ON"
                : "OFF"
        );
    }

    if (fiber_library_init() != 0) {
        printf(
            "[Application] Failed to initialize FiberLib.\n"
        );

        return;
    }

    for (int i = 0; i < count; i++) {
        ids[i] =
            fiber_create(
                custom_worker,
                &configs[i]
            );

        if (ids[i] == 0) {
            printf(
                "[Application] Failed to create fiber %d.\n",
                i + 1
            );

            fiber_library_shutdown();

            return;
        }

        int priority;

        printf(
            "Set priority for %s (1=LOW, 2=NORMAL, 3=HIGH): ",
            configs[i].name
        );

        if (scanf(
                "%d",
                &priority
            ) != 1) {

            printf(
                "[Application] Invalid priority. Using NORMAL.\n"
            );

            while (getchar() != '\n') {
            }

            priority = 2;
        }

        if (priority < 1 || priority > 3) {
            priority = 2;
        }

        fiber_set_priority(
            ids[i],
            (fiber_priority_t)priority
        );
    }

    printf(
        "\n[Application] All custom fibers created successfully.\n"
    );

    fiber_debug_dump();

    printf(
        "\n[Application] Starting scheduler...\n\n"
    );

    fiber_schedule();

    printf(
        "\n[Application] Custom fiber execution completed.\n"
    );

    fiber_stats_dump();

    fiber_library_shutdown();

    printf("\n");
}

/* =========================================================
 * Synchronization Demo
 *
 * Demonstrates cooperative mutual exclusion at the
 * application level using a shared lock owner.
 *
 * FiberLib currently provides fiber_yield() and fiber IDs,
 * so this demo builds a simple cooperative lock on top of
 * those primitives rather than pretending FiberLib already
 * contains a mutex API.
 * ========================================================= */

typedef struct {
    fiber_id_t owner;
} cooperative_lock_t;

typedef struct {
    const char *name;
    fiber_id_t id;
    cooperative_lock_t *lock;
    int *shared_counter;
} synchronization_worker_config_t;

static cooperative_lock_t sync_lock = {
    0
};

static int synchronization_counter = 0;

static int cooperative_lock_acquire(
    cooperative_lock_t *lock,
    fiber_id_t fiber_id,
    const char *fiber_name
)
{
    while (lock->owner != 0 &&
           lock->owner != fiber_id) {

        printf(
            "[Synchronization] %s waiting for the shared resource...\n",
            fiber_name
        );

        fiber_yield();
    }

    lock->owner = fiber_id;

    printf(
        "[Synchronization] %s acquired the lock.\n",
        fiber_name
    );

    return 0;
}

static void cooperative_lock_release(
    cooperative_lock_t *lock,
    fiber_id_t fiber_id,
    const char *fiber_name
)
{
    if (lock->owner == fiber_id) {
        lock->owner = 0;

        printf(
            "[Synchronization] %s released the lock.\n",
            fiber_name
        );
    }
}

static void synchronization_worker(void *arg)
{
    synchronization_worker_config_t *config =
        (synchronization_worker_config_t *)arg;

    printf(
        "\n[Synchronization] %s started.\n",
        config->name
    );

    for (int i = 1; i <= 3; i++) {
        printf(
            "[Synchronization] %s requesting shared resource "
            "(operation %d/3).\n",
            config->name,
            i
        );

        if (cooperative_lock_acquire(
                config->lock,
                config->id,
                config->name
            ) != 0) {

            printf(
                "[Synchronization] %s failed to acquire lock.\n",
                config->name
            );

            return;
        }

        printf(
            "[Synchronization] %s entered critical section.\n",
            config->name
        );

        int old_value =
            *(config->shared_counter);

        printf(
            "[Synchronization] %s reads shared counter = %d\n",
            config->name,
            old_value
        );

        /*
         * Yield while holding the lock.
         *
         * This makes the synchronization demonstration
         * visible: another fiber may run, but it cannot
         * enter the critical section because this fiber
         * still owns the lock.
         */
        fiber_yield();

        *(config->shared_counter) =
            old_value + 1;

        printf(
            "[Synchronization] %s updates shared counter: "
            "%d -> %d\n",
            config->name,
            old_value,
            *(config->shared_counter)
        );

        printf(
            "[Synchronization] %s leaving critical section.\n",
            config->name
        );

        cooperative_lock_release(
            config->lock,
            config->id,
            config->name
        );

        fiber_yield();
    }

    printf(
        "[Synchronization] %s completed all operations.\n",
        config->name
    );
}

static void synchronization_demo(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("           Synchronization Demo\n");
    printf("===============================================\n");

    printf(
        "[Demo] Two fibers will compete for one shared resource.\n"
    );

    printf(
        "[Demo] A cooperative lock protects the critical section.\n"
    );

    printf(
        "[Demo] The shared counter is updated only while a fiber\n"
        "       owns the lock.\n\n"
    );

    if (fiber_library_init() != 0) {
        printf(
            "[Demo] Failed to initialize FiberLib.\n"
        );

        return;
    }

    sync_lock.owner = 0;
    synchronization_counter = 0;

    synchronization_worker_config_t worker_a = {
        "Worker-A",
        0,
        &sync_lock,
        &synchronization_counter
    };

    synchronization_worker_config_t worker_b = {
        "Worker-B",
        0,
        &sync_lock,
        &synchronization_counter
    };

    fiber_id_t worker_a_id =
        fiber_create(
            synchronization_worker,
            &worker_a
        );

    fiber_id_t worker_b_id =
        fiber_create(
            synchronization_worker,
            &worker_b
        );

    if (worker_a_id == 0 ||
        worker_b_id == 0) {

        printf(
            "[Demo] Failed to create synchronization fibers.\n"
        );

        fiber_library_shutdown();

        return;
    }

    worker_a.id = worker_a_id;
    worker_b.id = worker_b_id;

    fiber_set_priority(
        worker_a_id,
        FIBER_PRIORITY_NORMAL
    );

    fiber_set_priority(
        worker_b_id,
        FIBER_PRIORITY_NORMAL
    );

    printf(
        "[Demo] Created Worker-A (Fiber %u) and "
        "Worker-B (Fiber %u).\n",
        worker_a_id,
        worker_b_id
    );

    printf(
        "[Demo] Both fibers will perform 3 protected operations.\n"
    );

    printf(
        "[Demo] Expected final counter value: 6\n\n"
    );

    fiber_debug_dump();

    printf(
        "\n[Demo] Starting synchronization demonstration...\n\n"
    );

    fiber_schedule();

    printf(
        "\n===============================================\n"
    );

    printf(
        "        Synchronization Result\n"
    );

    printf(
        "===============================================\n"
    );

    printf(
        "Final shared counter : %d\n",
        synchronization_counter
    );

    printf(
        "Expected counter      : 6\n"
    );

    if (synchronization_counter == 6) {
        printf(
            "Result                : PASS\n"
        );

        printf(
            "[Demo] All protected updates completed successfully.\n"
        );
    } else {
        printf(
            "Result                : FAIL\n"
        );

        printf(
            "[Demo] Shared counter did not reach the expected value.\n"
        );
    }

    printf("\n");

    fiber_stats_dump();

    fiber_library_shutdown();

    sync_lock.owner = 0;

    printf("\n");
}

/* =========================================================
 * Fiber Information Demo
 * ========================================================= */

typedef struct {
    const char *name;
    int steps;
} information_demo_config_t;

static void information_demo_worker(void *arg)
{
    information_demo_config_t *config =
        (information_demo_config_t *)arg;

    printf(
        "\n[Information Demo] %s started.\n",
        config->name
    );

    for (int i = 1; i <= config->steps; i++) {
        printf(
            "[Information Demo] %s executing step %d/%d\n",
            config->name,
            i,
            config->steps
        );

        fiber_yield();
    }

    printf(
        "[Information Demo] %s completed.\n",
        config->name
    );
}

static void fiber_information(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("              Fiber Information\n");
    printf("===============================================\n");

    printf(
        "[Info] This demonstration displays the internal\n"
        "       runtime information of active fibers.\n\n"
    );

    if (fiber_library_init() != 0) {
        printf(
            "[Info] Unable to initialize FiberLib.\n"
        );

        return;
    }

    information_demo_config_t fiber_a_config = {
        "Information-A",
        2
    };

    information_demo_config_t fiber_b_config = {
        "Information-B",
        3
    };

    information_demo_config_t fiber_c_config = {
        "Information-C",
        2
    };

    fiber_id_t fiber_a =
        fiber_create(
            information_demo_worker,
            &fiber_a_config
        );

    fiber_id_t fiber_b =
        fiber_create(
            information_demo_worker,
            &fiber_b_config
        );

    fiber_id_t fiber_c =
        fiber_create(
            information_demo_worker,
            &fiber_c_config
        );

    if (fiber_a == 0 ||
        fiber_b == 0 ||
        fiber_c == 0) {

        printf(
            "[Info] Failed to create information-demo fibers.\n"
        );

        fiber_library_shutdown();

        return;
    }

    fiber_set_priority(
        fiber_a,
        FIBER_PRIORITY_HIGH
    );

    fiber_set_priority(
        fiber_b,
        FIBER_PRIORITY_NORMAL
    );

    fiber_set_priority(
        fiber_c,
        FIBER_PRIORITY_LOW
    );

    printf(
        "[Info] Created three demonstration fibers.\n"
    );

    printf(
        "[Info] Priority configuration:\n"
    );

    printf(
        "       Fiber %u -> HIGH\n",
        fiber_a
    );

    printf(
        "       Fiber %u -> NORMAL\n",
        fiber_b
    );

    printf(
        "       Fiber %u -> LOW\n",
        fiber_c
    );

    printf(
        "\n[Info] Fiber state before scheduling:\n\n"
    );

    fiber_debug_dump();

    printf(
        "\n[Info] Information displayed by FiberLib includes:\n"
    );

    printf(
        "       - Fiber ID (TID)\n"
    );

    printf(
        "       - Lifecycle state\n"
    );

    printf(
        "       - Scheduling priority\n"
    );

    printf(
        "       - Context switch count\n"
    );

    printf(
        "       - Aging value\n"
    );

    printf(
        "       - Allocated stack size\n"
    );

    printf(
        "\n[Info] Starting fibers so their runtime state can be observed...\n\n"
    );

    fiber_schedule();

    printf(
        "\n[Info] All information-demo fibers have finished.\n"
    );

    printf(
        "\n[Info] Final FiberLib state:\n\n"
    );

    fiber_debug_dump();

    printf(
        "\n[Info] Runtime statistics:\n\n"
    );

    fiber_stats_dump();

    fiber_library_shutdown();

    printf("\n");
}

/* =========================================================
 * Feature List
 * ========================================================= */

static void show_features(void)
{
    printf("\n");
    printf("===============================================\n");
    printf("             FiberLib Features\n");
    printf("===============================================\n");

    printf(
        "1.  User-level fiber creation\n"
    );

    printf(
        "2.  Manual stack allocation\n"
    );

    printf(
        "3.  POSIX ucontext-based context switching\n"
    );

    printf(
        "4.  Cooperative Round-Robin scheduling\n"
    );

    printf(
        "5.  Priority-aware scheduling\n"
    );

    printf(
        "6.  Fiber lifecycle management\n"
    );

    printf(
        "7.  Fiber join / waiting\n"
    );

    printf(
        "8.  Deadlock detection\n"
    );

    printf(
        "9.  Priority aging for starvation prevention\n"
    );

    printf(
        "10. Fiber runtime statistics\n"
    );

    printf(
        "11. Fiber state debugging\n"
    );

    printf(
        "12. Interactive scheduling demonstration\n"
    );

    printf(
        "13. Priority scheduling demonstration\n"
    );

    printf(
        "14. Priority aging demonstration\n"
    );

    printf(
        "15. Cooperative synchronization demonstration\n"
    );

    printf(
        "16. Interactive application interface\n"
    );

    printf("\n");
}

/* =========================================================
 * Main Menu
 * ========================================================= */

int main(void)
{
    int choice;

    while (1) {
        printf("\n");
        printf("===============================================\n");
        printf("                 FiberLib\n");
        printf("          User-Level Thread Library\n");
        printf("===============================================\n");

        printf(
            "1. Run FiberLib Application\n"
        );

        printf(
            "2. Create Custom Fiber(s)\n"
        );

        printf(
            "3. Run Scheduling Demo\n"
        );

        printf(
            "4. Run Synchronization Demo\n"
        );

        printf(
            "5. View Fiber Information\n"
        );

        printf(
            "6. Show Library Features\n"
        );

        printf(
            "7. Priority Scheduling Demo\n"
        );

        printf(
            "8. Priority Aging Demo\n"
        );

        printf(
            "9. Exit\n"
        );

        printf(
            "===============================================\n"
        );

        printf(
            "Enter your choice: "
        );

        if (scanf(
                "%d",
                &choice
            ) != 1) {

            printf(
                "\n[Application] Invalid input. "
                "Please enter a number.\n"
            );

            while (getchar() != '\n') {
            }

            continue;
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
                priority_scheduling_demo();
                break;

            case 8:
                priority_aging_demo();
                break;

            case 9:
                printf(
                    "\nExiting FiberLib. Goodbye!\n"
                );

                return 0;

            default:
                printf(
                    "\n[Application] Invalid choice. "
                    "Please select 1-9.\n"
                );
        }
    }
}
