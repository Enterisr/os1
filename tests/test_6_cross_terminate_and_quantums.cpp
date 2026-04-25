#include "uthreads.h"

#include <iostream>
#include <limits>

static int victim_tid = -1;
static int killer_tid = -1;

static void victim() {
    std::cout << "victim start tid=" << uthread_get_tid()
              << " total=" << uthread_get_total_quantums()
              << " q=" << uthread_get_quantums(uthread_get_tid())
              << std::endl;

    if (uthread_sleep(0) != 0) {
        std::cout << "victim sleep failed" << std::endl;
        exit(1);
    }

    // Should never get here (expected to be terminated by killer).
    std::cout << "victim resumed unexpectedly" << std::endl;
    exit(1);
}

static void killer() {
    std::cout << "killer start tid=" << uthread_get_tid()
              << " total=" << uthread_get_total_quantums()
              << " q=" << uthread_get_quantums(uthread_get_tid())
              << " victim_q=" << uthread_get_quantums(victim_tid)
              << std::endl;

    int err = uthread_terminate(victim_tid);
    std::cout << "killer terminate victim err=" << err << std::endl;

    if (uthread_sleep(0) != 0) {
        std::cout << "killer sleep failed" << std::endl;
        exit(1);
    }

    std::cout << "killer resumed total=" << uthread_get_total_quantums()
              << " q=" << uthread_get_quantums(killer_tid)
              << std::endl;

    // Yield back to main; main will externally terminate us.
    if (uthread_sleep(0) != 0) {
        std::cout << "killer sleep2 failed" << std::endl;
        exit(1);
    }

    // Should never get here (expected to be terminated by main).
    std::cout << "killer resumed unexpectedly" << std::endl;
    exit(1);
}

int main() {
    int result = uthread_init(std::numeric_limits<int>::max());
    if (result != 0) {
        std::cout << "uthread_init failed with code " << result << std::endl;
        return 1;
    }

    victim_tid = uthread_spawn(victim);
    killer_tid = uthread_spawn(killer);

    if (victim_tid == -1 || killer_tid == -1) {
        std::cout << "spawn failed" << std::endl;
        return 1;
    }

    std::cout << "main init total=" << uthread_get_total_quantums()
              << " q0=" << uthread_get_quantums(0)
              << " victim=" << victim_tid
              << " killer=" << killer_tid
              << std::endl;

    // Start scheduling: main -> victim -> killer -> main.
    if (uthread_sleep(0) != 0) {
        std::cout << "main sleep failed" << std::endl;
        return 1;
    }

    std::cout << "main after kill total=" << uthread_get_total_quantums()
              << " q0=" << uthread_get_quantums(0)
              << " killer_q=" << uthread_get_quantums(killer_tid)
              << " victim_q=" << uthread_get_quantums(victim_tid)
              << std::endl;

    // main -> killer (resume) -> main.
    if (uthread_sleep(0) != 0) {
        std::cout << "main sleep2 failed" << std::endl;
        return 1;
    }

    // killer -> main (killer yielded). Terminate killer externally.
    if (uthread_terminate(killer_tid) != 0) {
        std::cout << "terminate killer failed" << std::endl;
        return 1;
    }

    std::cout << "main end total=" << uthread_get_total_quantums()
              << " q0=" << uthread_get_quantums(0)
              << " killer_q=" << uthread_get_quantums(killer_tid)
              << std::endl;

    std::cout << "Success" << std::endl;
    return 0;
}
