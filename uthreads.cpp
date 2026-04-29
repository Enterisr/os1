#include "uthreads.h"
#include <iostream>
#include <algorithm>
#include <queue>
#include <vector>
#include <memory>

#include "ThreadIDManager.h"
#include "Thread.h"


static int quantum_duration;
static ThreadIDManager id_manager;
static std::unique_ptr<Thread> threads[MAX_THREAD_NUM];
static std::deque<int> ready_queue;
static std::unique_ptr<Thread> pending_deletion;
static int running_thread=0;
static int quantum_count=1;
static void switch_to_next(bool save_current);
[[noreturn]] static void terminate_self();
static bool validate_thread(int tid);
static void load_next_thread_context();


/**
 * @brief initializes the thread library.
 *
 * Once this function returns, the main thread (tid == 0) will be set as RUNNING. There is no need to 
 * provide an entry_point or to create a stack for the main thread - it will be using the "regular" stack and PC.
 * You may assume that this function is called before any other thread library function, and that it is called
 * exactly once.
 * The input to the function is the length of a quantum in micro-seconds.
 * It is an error to call this function with non-positive quantum_usecs.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs) {
    if(quantum_usecs<1){
        std::cerr << "thread library error: quantum_usecs must be positive\n";
        return -1;
    }
    quantum_duration  = quantum_usecs;
    threads[0] = std::make_unique<Thread>(0);
    return 0; 
}

/**
 * @brief Creates a new thread, whose entry point is the function entry_point with the signature
 * void entry_point(void).
 *
 * The thread is added to the end of the READY threads list.
 * The uthread_spawn function should fail if it would cause the number of concurrent threads to exceed the
 * limit (MAX_THREAD_NUM).
 * Each thread should be allocated with a stack of size STACK_SIZE bytes.
 * It is an error to call this function with a null entry_point.
 *
 * @return On success, return the ID of the created thread. On failure, return -1.
*/
int uthread_spawn(thread_entry_point entry_point) {
    if(entry_point==nullptr){
        std::cerr << "thread library error: entry point can't be null\n";
        return -1;
    }

    int id =  id_manager.allocateID();
    if(MAX_THREAD_NUM<=id){
        std::cerr << "thread library error: too much threads  \n";
        return -1;
    }

    try {
        threads[id] = std::make_unique<Thread>(id, entry_point);
        ready_queue.push_back(id); 
    } catch (const std::bad_alloc&) {
        std::cerr << "thread library error: memory allocation failed\n";
        id_manager.deallocateID(id);
        return -1;
    }
    return id;
}


/**
 * @brief Terminates the thread with ID tid and deletes it from all relevant control structures.
 *
 * All the resources allocated by the library for this thread should be released. If no thread with ID tid exists it
 * is considered an error. Terminating the main thread (tid == 0) will result in the termination of the entire
 * process using exit(0) (after releasing the assigned library memory).
 *
 * @return The function returns 0 if the thread was successfully terminated and -1 otherwise. If a thread terminates
 * itself or the main thread is terminated, the function does not return.
*/
int uthread_terminate(int tid){

    if(threads[tid]==nullptr){
        std::cerr << "thread library error: " << "no thread with id: "<<tid << std::endl;
        return -1;
    }
    if (tid == 0) {
        for (int i = 0; i < MAX_THREAD_NUM; i++) {
            threads[i].reset();
        }
        pending_deletion.reset();
        exit(0); 
    }
    if (tid == running_thread){
        terminate_self();
    }
    id_manager.deallocateID(tid);
    ready_queue.erase(
        std::remove(ready_queue.begin(), ready_queue.end(), tid),
        ready_queue.end());
    threads[tid].reset();
    return 0;
}

/**
 * @brief Blocks the thread with ID tid. The thread may be resumed later using uthread_resume.
 *
 * If no thread with ID tid exists it is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision should be made. Blocking a thread in
 * BLOCKED state has no effect and is *not* considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_block(int tid) {
    if(!validate_thread(tid)) return -1;
    
    if(tid == 0){ //trying to block the main thread 
        std::cerr << "thread library error: " << "can not block main thread" << std::endl;
        return -1;
    }
    Thread* t = threads[tid].get();
    if (t->state == BLOCKED) return 0;
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


/**
 * @brief Resumes a blocked thread with ID tid and moves it to the READY state.
 *
 * Resuming a thread in a RUNNING or READY state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * When a thread transition to the READY state it is placed at the end of the READY queue.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid) {
    if(!validate_thread(tid)) return -1;
    Thread* t = threads[tid].get();
    if (t->state != BLOCKED) return 0;
    t->state = READY;
    if (t->sleep_remaining == 0){
        ready_queue.push_back(tid);
    }
   
    return 0;
}
static std::vector<int> tick_sleepers(){
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
static void load_next_thread_context(){
    quantum_count++;
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

[[noreturn]] static void terminate_self() {
    int tid = running_thread;
    id_manager.deallocateID(tid);
    pending_deletion = std::move(threads[tid]);
    switch_to_next(false);   // picks next, longjmps in, never returns
    exit(1);                       // safety net, should be unreachable
}

/**
 * @brief Blocks the RUNNING thread for num_quantums quantums.
 *
 * Immediately after the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY queue.
 * If the thread which was just RUNNING should also be added to the READY queue, or if multiple threads wake up 
 * at the same time, the order in which they're added to the end of the READY queue doesn't matter.
 * The number of quantums refers to the number of times a new quantum starts, regardless of the reason. Specifically,
 * the quantum of the thread which has made the call to uthread_sleep isn’t counted.
 * A call with num_quantums == 0 will immediately stop the thread and move it to the back of the execution queue.
 * 
 * It is considered an error if the main thread (tid == 0) calls this function with num_quantums != 0.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums) {
    if(num_quantums<0){
        std::cerr << "thread library error: negative sleep duration\n";
        return -1;
    }
        if (num_quantums > 0 && running_thread == 0) {
        std::cerr << "thread library error: main cannot sleep with N>0\n";
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


/**
 * @brief Returns the thread ID of the calling thread.
 *
 * @return The ID of the calling thread.
*/
int uthread_get_tid() {
    return running_thread;
}


/**
 * @brief Returns the total number of quantums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantums.
*/
int uthread_get_total_quantums() {
    return quantum_count;
}


/**
 * @brief Returns the number of quantums the thread with ID tid was in RUNNING state.
 *
 * On the first time a thread runs, the function should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state when this function is called, include
 * also the current quantum). If no thread with ID tid exists it is considered an error.
 *
 * @return On success, return the number of quantums of the thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums(int tid) {
    if(threads[tid]==nullptr){
        std::cerr << "thread library error: " << "no thread with id: "<<tid << std::endl;
        return -1;
    }
    return threads[tid]->get_quantum_count();
}

static void switch_to_next(bool save_current) {
    if (save_current) {
        if (sigsetjmp(threads[running_thread]->env, 1) != 0) {
            pending_deletion.reset();
            return;
        }

    }
    // pick next, 
    load_next_thread_context();
}


static bool validate_thread(int tid){
        if(threads[tid]==nullptr){
        std::cerr << "thread library error: " << "no thread with id: "<<tid << std::endl;
        return false;
    }
    return true;
}



