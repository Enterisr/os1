#include "Scheduler.h"
#include <iostream>
#include <algorithm>
    

Scheduler* Scheduler::instance = nullptr;

// ==============================================================================
// Singleton Management & Constructor
// ==============================================================================

Scheduler::Scheduler(int quantum_usecs) : timer(quantum_usecs) {
    this->running_thread = 0;
    this->total_quantums = 1;
    this->quanta_duration = quantum_usecs;
    threads[0] = std::make_unique<Thread>(0);
    timer.start();
}

 void Scheduler::init(int quantum_usecs) {
    if (instance == nullptr) {
        instance = new Scheduler(quantum_usecs);
        instance->timer.unblock_timer_signal();
    }
}

Scheduler* Scheduler::get_instance() {
    return instance;
}

[[noreturn]] void Scheduler::cleanup_and_exit(int code) {
    for (int i = 0; i < MAX_THREAD_NUM; i++) {
        threads[i].reset();
    }
    pending_deletion.reset();
    exit(code);
}

//  API implementations (called by uthreads.cpp)
int Scheduler::spawn(thread_entry_point entry_point) {
    timer.block_timer_signal();
    if(entry_point==nullptr){
        std::cerr << "thread library error: entry point can't be null\n";
        timer.unblock_timer_signal();
        return -1;
    }

    int id =  id_manager.allocateID();
    if(MAX_THREAD_NUM<=id){
        std::cerr << "thread library error: too much threads  \n";
        timer.unblock_timer_signal();
        return -1;
    }

    try {
        threads[id] = std::make_unique<Thread>(id, entry_point);
        ready_queue.push_back(id); 
    } catch (const std::bad_alloc&) {
        std::cerr << "thread library error: memory allocation failed\n";
        id_manager.deallocateID(id);
        timer.unblock_timer_signal();
        return -1;
    }
    timer.unblock_timer_signal();
    return id;

}

int Scheduler::terminate(int tid) {
    timer.block_timer_signal();
    if(threads[tid]==nullptr){
        std::cerr << "thread library error: " << "no thread with id: "<<tid << std::endl;
        timer.unblock_timer_signal();
        return -1;
    }
    if (tid == 0) {
        if (running_thread == 0) {
            cleanup_and_exit(0);
        }

        // erminate(0) is called from  non-main thread: dont free the current
        // running thread's stack while executing on it. pass to main.
        shutdown_requested = true;
        shutdown_exit_code = 0;

        pending_deletion = std::move(threads[running_thread]);

        ready_queue.erase(
            std::remove(ready_queue.begin(), ready_queue.end(), 0),
            ready_queue.end());
        threads[0]->state = READY;
        ready_queue.push_front(0);

        switch_to_next(false); // longjmps into main.. never returns
        exit(1);
    }
    if (tid == running_thread){
        terminate_self();
    }
    id_manager.deallocateID(tid);
    ready_queue.erase(
        std::remove(ready_queue.begin(), ready_queue.end(), tid),
        ready_queue.end());
    threads[tid].reset();
    timer.unblock_timer_signal();
    return 0;
}

int Scheduler::block(int tid) {
    timer.block_timer_signal();
    if(!validate_thread(tid)){
    timer.unblock_timer_signal();
    return -1;
    }
    if(tid == 0){ //trying to block the BOSS thread
        std::cerr << "thread library error: " << "can not block main thread" << std::endl;
        timer.unblock_timer_signal();
        return -1;
    }
    Thread* t = threads[tid].get();
    if (t->state == BLOCKED) {
        timer.unblock_timer_signal();
        return 0;
    };
    t->state = BLOCKED;
    if (tid == running_thread){
        switch_to_next(true);
    } else{
    ready_queue.erase(
        std::remove(ready_queue.begin(), ready_queue.end(), tid),
        ready_queue.end());
    }
    return 0;
}

int Scheduler::resume(int tid) {
    timer.block_timer_signal();
    if(!validate_thread(tid)){
        timer.unblock_timer_signal();
        return -1;
    } 
    Thread* t = threads[tid].get();
    if (t->state != BLOCKED) {
        timer.unblock_timer_signal();
        return 0;
    };
    t->state = READY;
    if (t->sleep_remaining == 0){
        ready_queue.push_back(tid);
    }  
    timer.unblock_timer_signal();
    return 0;
}

int Scheduler::sleep(int num_quantums) {
    timer.block_timer_signal();
    if(num_quantums<0){
        std::cerr << "thread library error: negative sleep duration\n";
        timer.unblock_timer_signal();
        return -1;
    }
    if (num_quantums > 0 && running_thread == 0) {
        std::cerr << "thread library error: main cannot sleep with N>0\n";
        timer.unblock_timer_signal();
        return -1;
    }
    if (num_quantums == 0) {
        threads[running_thread]->state = READY;
        ready_queue.push_back(running_thread);
        switch_to_next(true);
        return 0;
    }
    threads[running_thread]->state = READY;
    threads[running_thread]->sleep_remaining = num_quantums;
    switch_to_next(true);
    return 0;
}

int Scheduler::get_running_thread() {
    return running_thread;
}

int Scheduler::get_total_quantums() {
    return total_quantums;
}

int Scheduler::get_thread_quantums(int tid) {
    if(threads[tid]==nullptr){
        std::cerr << "thread library error: " << "no thread with id: "<<tid << std::endl;
        return -1;
    }
    return threads[tid]->get_quantum_count();
}

// ==============================================================================
// Timer Interrupt Bridge
// ==============================================================================

void Scheduler::handle_timer_interrupt() {
    threads[running_thread]->state = READY;
    ready_queue.push_back(running_thread);
    switch_to_next(true);
}

// private helper methods for context switching and sleeping
void Scheduler::switch_to_next(bool save_current) {
    if (save_current) {
        if (sigsetjmp(threads[running_thread]->env, 1) != 0) {
            pending_deletion.reset();

            if (shutdown_requested && running_thread == 0) {
                cleanup_and_exit(shutdown_exit_code);
            }

            timer.unblock_timer_signal();
            return;
        }

    }
    // pick next,
    load_next_thread_context();
}

void Scheduler::load_next_thread_context() {
    total_quantums++;
    std::vector<int> just_woke = tick_sleepers();
    int next_t_idx = -1;
    if(!ready_queue.empty()){
        //context switch
        next_t_idx = ready_queue.front();
        ready_queue.pop_front();
    }
    for (int tid : just_woke) {
        ready_queue.push_back(tid);
    }
    if (next_t_idx != -1) {
        running_thread = next_t_idx;
        threads[next_t_idx]->on_RUNNING();
    }
}

std::vector<int> Scheduler::tick_sleepers() {
        std::vector<int> just_woke;
        for (int i = 0; i < MAX_THREAD_NUM; i++) {
        if (threads[i] && threads[i]->sleep_remaining > 0) {
            threads[i]->sleep_remaining--;
            if (threads[i]->sleep_remaining == 0 && threads[i]->state == READY) {
                just_woke.push_back(i);
            }
        }
    }
    return just_woke;
}

bool Scheduler::validate_thread(int tid) {
        if(threads[tid]==nullptr){
        std::cerr << "thread library error: " << "no thread with id: "<<tid << std::endl;
        return false;
    }
    return true;}

void Scheduler::terminate_self() {
    int tid = running_thread;
    id_manager.deallocateID(tid);
    pending_deletion = std::move(threads[tid]);
    switch_to_next(false);   // picks next, longjmps in, never returns
    exit(1);                       // Ssafety net, should be unreachable
}