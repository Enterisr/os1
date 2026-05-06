#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <deque>
#include <unordered_map>
#include "Timer.h"
#include "Thread.h"     
#include "ThreadIDManager.h"

class Scheduler {
    private:
        Scheduler(int quantum_usecs);
        static Scheduler* instance;
        std::unique_ptr<Thread> threads[MAX_THREAD_NUM];
        std::deque<int> ready_queue;
        std::unique_ptr<Thread> pending_deletion;

        bool shutdown_requested = false;
        int shutdown_exit_code = 0;
        
        int running_thread;
        int total_quantums;
        int quanta_duration;
        ThreadIDManager id_manager;
        Timer timer;

        void perform_context_switch();
        void switch_to_next(bool save_current);
        void load_next_thread_context();
        std::vector<int> tick_sleepers();
        bool validate_thread(int tid);
        void terminate_self();
        [[noreturn]] void cleanup_and_exit(int code);

    public:
        Scheduler(const Scheduler&) = delete;
        Scheduler& operator=(const Scheduler&) = delete;

        static void init(int quantum_usecs);
        static Scheduler* get_instance();

        ~Scheduler();

        int spawn(void (*f)(void));
        int terminate(int tid);
        int block(int tid);
        int resume(int tid);
        int sleep(int num_quantums);
        
        int get_running_thread();
        int get_total_quantums();
        int get_thread_quantums(int tid);

        void handle_timer_interrupt(); 
};

#endif