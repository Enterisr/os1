#pragma once
#include <setjmp.h>
#include <memory>
#include "uthreads.h"

enum ThreadState { READY, RUNNING, SLEEP };

class Thread {
    int id;
    ThreadState state;
    std::unique_ptr<char[]> stack;
    int quantum_count;

    public:
        sigjmp_buf env;
        int get_quantum_count();
        void on_RUNNING();
        void on_SLEEP();
        Thread(int id,thread_entry_point entry_point);
        // id(id), 
        // entry(entry_point),
        // state(READY),
        // stack(std::make_unique<char[]>(STACK_SIZE)),
        // quantum_count(0)
        // {
        //  address_t sp = (address_t)stack+STACK_SIZE-sizeof(address_t);;   
        // }
        //main thread cnstrcr
        explicit Thread(int id) :
        id(id), state(RUNNING),
        stack(nullptr),  //main thread
        quantum_count(1) {}
};