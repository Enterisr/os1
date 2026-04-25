#include "Thread.h"
#include "utils.h"
#include <signal.h>
Thread::Thread(int id, thread_entry_point entry_point) {
    this->id = id;
    state = READY;
    quantum_count = 0;
    stack = std::make_unique<char[]>(STACK_SIZE);
    address_t sp = (address_t)stack.get() + STACK_SIZE - sizeof(address_t);
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
void Thread::on_SLEEP(){
    state = SLEEP;
    sigsetjmp(env,1);
}
int Thread::get_quantum_count()  { return quantum_count; }
