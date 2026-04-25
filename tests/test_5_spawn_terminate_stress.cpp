#include "uthreads.h"
#include <iostream>
#include <limits>

static void do_nothing() {}

int main() {
    int result = uthread_init(std::numeric_limits<int>::max());
    if (result != 0) {
        std::cout << "uthread_init failed with code " << result << std::endl;
        return 1;
    }

    // Repeated spawn/terminate is a good canary for leaks
    // (run this under valgrind/asan externally if desired).
    const int iterations = 2000;
    int failures = 0;

    for (int i = 1; i <= iterations; i++) {
        int tid = uthread_spawn(do_nothing);
        if (tid == -1) {
            failures++;
            break;
        }

        int err = uthread_terminate(tid);
        if (err != 0) {
            failures++;
            break;
        }

        if (i % 500 == 0) {
            std::cout << "progress " << i << "/" << iterations << std::endl;
        }
    }

    std::cout << "failures: " << failures << std::endl;
    std::cout << "Success" << std::endl;
    return failures == 0 ? 0 : 1;
}
