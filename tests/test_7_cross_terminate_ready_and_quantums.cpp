#include "uthreads.h"

#include <iostream>
#include <limits>

static int victim_tid = -1;
static int killer_tid = -1;

static void victim() {
    std::cout << "victim ran unexpectedly tid=" << uthread_get_tid() << std::endl;
    exit(1);
}

static void killer() {
    const int tid = uthread_get_tid();

    const int total_before = uthread_get_total_quantums();
    const int killer_q_before = uthread_get_quantums(tid);
    const int victim_q_before = uthread_get_quantums(victim_tid);

    std::cout << "killer start tid=" << tid
              << " total=" << total_before
              << " q=" << killer_q_before
              << " victim_q0=" << victim_q_before
              << std::endl;

    int err = uthread_terminate(victim_tid);

    const int victim_q_after = uthread_get_quantums(victim_tid);
    std::cout << "killer terminate victim err=" << err
              << " victim_q1=" << victim_q_after
              << std::endl;

    if (uthread_sleep(0) != 0) {
        std::cout << "killer sleep failed" << std::endl;
        exit(1);
    }

    const int total_after = uthread_get_total_quantums();
    const int killer_q_after = uthread_get_quantums(killer_tid);

    std::cout << "killer resumed total=" << total_after
              << " q=" << killer_q_after
              << std::endl;

    // Yield back to main; main will externally terminate us.
    if (uthread_sleep(0) != 0) {
        std::cout << "killer sleep2 failed" << std::endl;
        exit(1);
    }

    std::cout << "killer resumed unexpectedly" << std::endl;
    exit(1);
}

int main() {
    int result = uthread_init(std::numeric_limits<int>::max());
    if (result != 0) {
        std::cout << "uthread_init failed with code " << result << std::endl;
        return 1;
    }

    // Spawn killer first so it runs before victim (FIFO ready queue).
    killer_tid = uthread_spawn(killer);
    victim_tid = uthread_spawn(victim);

    if (victim_tid == -1 || killer_tid == -1) {
        std::cout << "spawn failed" << std::endl;
        return 1;
    }

    const int total0 = uthread_get_total_quantums();
    const int q0 = uthread_get_quantums(0);

    std::cout << "main init total=" << total0
              << " q0=" << q0
              << " killer=" << killer_tid
              << " victim=" << victim_tid
              << std::endl;

    // main -> killer -> main.
    if (uthread_sleep(0) != 0) {
        std::cout << "main sleep failed" << std::endl;
        return 1;
    }

    const int total1 = uthread_get_total_quantums();
    const int q0_1 = uthread_get_quantums(0);
    const int killer_q = uthread_get_quantums(killer_tid);
    const int victim_q = uthread_get_quantums(victim_tid);

    std::cout << "main after kill total=" << total1
              << " q0=" << q0_1
              << " killer_q=" << killer_q
              << " victim_q=" << victim_q
              << std::endl;

    // main -> killer(resume) -> main.
    if (uthread_sleep(0) != 0) {
        std::cout << "main sleep2 failed" << std::endl;
        return 1;
    }

    // killer -> main (killer yielded). Terminate killer externally.
    if (uthread_terminate(killer_tid) != 0) {
        std::cout << "terminate killer failed" << std::endl;
        return 1;
    }

    const int total2 = uthread_get_total_quantums();
    const int q0_2 = uthread_get_quantums(0);
    const int killer_q2 = uthread_get_quantums(killer_tid);

    std::cout << "main end total=" << total2
              << " q0=" << q0_2
              << " killer_q=" << killer_q2
              << std::endl;

    std::cout << "Success" << std::endl;
    return 0;
}
