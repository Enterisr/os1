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

    // Terminate a thread that does not exist (but tid is in-range).
    int err = uthread_terminate(1);
    std::cout << "terminate(nonexistent tid=1): " << err << std::endl;

    // Spawn + terminate should succeed.
    int tid = uthread_spawn(do_nothing);
    std::cout << "spawn(do_nothing) tid: " << tid << std::endl;
    if (tid == -1) {
        return 1;
    }

    err = uthread_terminate(tid);
    std::cout << "terminate(existing tid=" << tid << "): " << err << std::endl;

    // Double-terminate should fail.
    err = uthread_terminate(tid);
    std::cout << "terminate(already-terminated tid=" << tid << "): " << err << std::endl;

    // get_quantums() on a non-existent tid should fail.
    int q = uthread_get_quantums(42);
    std::cout << "get_quantums(nonexistent tid=42): " << q << std::endl;

    // Spawning with a nullptr entry point should fail.
    int null_tid = uthread_spawn(nullptr);
    std::cout << "spawn(nullptr): " << null_tid << std::endl;

    std::cout << "Success" << std::endl;
    return 0;
}
