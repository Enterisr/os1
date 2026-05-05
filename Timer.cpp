#include "Timer.h"
#include "Scheduler.h"
#include <iostream>
#include <sys/time.h>
#include <signal.h>

Timer::Timer(int quantum_usecs) {
    sigemptyset(&timer_set);
    sigaddset(&timer_set, SIGVTALRM);
    this->quantum_usecs = quantum_usecs;
    struct sigaction sa = {0};
    sa.sa_handler = &Timer::timer_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGVTALRM); 
    
    if(sigaction(SIGVTALRM, &sa, nullptr)){
        std::cerr << "system error: sigaction failed\n";
        exit(1);
    }
    sigemptyset(&timer_set);
    sigaddset(&timer_set, SIGVTALRM);
}

Timer::~Timer() {
    stop();
}


void Timer::start() {
    reset();
}

void Timer::stop() {

    struct itimerval stop_timer = {0};
    if (setitimer(ITIMER_VIRTUAL, &stop_timer, nullptr)<0){
        std::cerr << "system error: [stop] setitimer failed\n";
        exit(1);
    }
}

void Timer::reset() {

    struct itimerval timer;
    int secs = quantum_usecs / 1000000;
    int usecs = quantum_usecs % 1000000;

    timer.it_value.tv_sec = secs;
    timer.it_value.tv_usec = usecs;
    timer.it_interval.tv_sec = secs;
    timer.it_interval.tv_usec = usecs;


    if (setitimer(ITIMER_VIRTUAL, &timer, nullptr) < 0) {
        std::cerr << "system error: setitimer failed\n";
        exit(1);
    }
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