#include "Timer.h"
#include "Scheduler.h"
#include <iostream>
#include <sys/time.h>
#include <signal.h>

Timer::Timer(int quantum_usecs) {
    // 1. Initialize the signal set (timer_set)
    sigemptyset(&timer_set);
    sigaddset(&timer_set, SIGVTALRM);

    // 2. Configure the sigaction structure
    struct sigaction sa = {0};
    sa.sa_handler = &Timer::timer_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGVTALRM); 
    
    // 3. Register the signal handler with the OS
    if(sigaction(SIGVTALRM, &sa, nullptr)){
        std::cerr << "system error: sigaction failed\n";
        exit(1);
    }
    
    timer.it_value.tv_sec = quantum_usecs / 1000000;
    timer.it_value.tv_usec = quantum_usecs % 1000000;
    timer.it_interval.tv_sec = quantum_usecs / 1000000;
    timer.it_interval.tv_usec = quantum_usecs % 1000000;
}

Timer::~Timer() {
    // Optional: You can call stop() here to cleanly disable the timer on shutdown
    stop();
}

// ==============================================================================
// Timer Controls
// ==============================================================================

void Timer::start() {
    if(setitimer(ITIMER_VIRTUAL, &timer, nullptr)<0){
        std::cerr << "system error: [start] setitimer failed\n";
        exit(1);
    }
}

void Timer::stop() {
    struct itimerval stop_timer = {0};
    if (setitimer(ITIMER_VIRTUAL, &stop_timer, nullptr)<0){
        std::cerr << "system error: [stop] setitimer failed\n";
        exit(1);
    }
}

void Timer::reset() {
    start(); 
}


void Timer::block_timer_signal() {
    if(sigprocmask(SIG_BLOCK, &timer_set, nullptr)<0){
        std::cerr << "system error: [block] sigprocmask failed\n";
        exit(1);
    }
}

void Timer::unblock_timer_signal() {
    if(sigprocmask(SIG_UNBLOCK, &timer_set, nullptr)<0){
        std::cerr << "system error: block sigprocmask failed\n";
        exit(1);
    }
}


void Timer::timer_handler(int sig) {
    Scheduler::get_instance()->handle_timer_interrupt();
}