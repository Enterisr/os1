#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>
#include <signal.h>

class Timer {
private:
    struct sigaction sa;
    struct itimerval timer;
    sigset_t timer_set; 
    int quantum_usecs;

    static void timer_handler(int sig);

public:
    Timer(int quantum_usecs);
    ~Timer();

    void start();

    void stop();

    void reset();

    void block_timer_signal();
    void unblock_timer_signal();
};

#endif // TIMER_H