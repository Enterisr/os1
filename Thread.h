#pragma once
#include <setjmp.h>
#include <memory>
#include "uthreads.h"

enum ThreadState { READY, RUNNING, BLOCKED};

class Thread {
    int id;
    std::unique_ptr<char[]> stack;
    int quantum_count;


    public:
        int sleep_remaining;
        ThreadState state;
        sigjmp_buf env;
        int get_quantum_count();
        void on_RUNNING();
        void save_context();
        static void switch_to_next(bool save_current);
        // void on_SLEEP();
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
        explicit Thread(int id) 
            : id(id),
                stack(nullptr),  //main thread
                quantum_count(1),
                sleep_remaining(0),
                state(RUNNING)

        {}
};