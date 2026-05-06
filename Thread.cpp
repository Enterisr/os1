#include "Thread.h"
#include "utils.h"
#include <signal.h>
#include <iostream>

static size_t effective_stack_size() {
    // ASAN adds large red-zones and can easily overflow a 4KB user-level stack.
    // Student ASAN tests explicitly suggest increasing the stack size.
#if defined(__SANITIZE_ADDRESS__)
    return 100000;
#elif defined(__has_feature)
#  if __has_feature(address_sanitizer)
    return 100000;
#  else
    return STACK_SIZE;
#  endif
#else
    return STACK_SIZE;
#endif
}

Thread::Thread(int id, thread_entry_point entry_point) {
    this->id = id;
    state = READY;
    sleep_remaining = 0;        
    quantum_count = 0;
    const size_t stack_size = effective_stack_size();
    stack = std::make_unique<char[]>(stack_size);
    address_t sp = (address_t)stack.get() + stack_size - sizeof(address_t);
    address_t pc = (address_t)entry_point;
    sigsetjmp(env, 1);
    (env->__jmpbuf)[JB_SP] = translate_address(sp);
    (env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env->__saved_mask);
}
void Thread::on_RUNNING(){
    quantum_count++;
    state = RUNNING;
    siglongjmp(env,1);
}
void Thread::save_context(){
    sigsetjmp(env,1);
}
int Thread::get_quantum_count()  { return quantum_count; }
