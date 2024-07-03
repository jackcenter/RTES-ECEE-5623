#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#define NUM_TIMERS 3  // Example: Handling 3 timers

timer_t timers[NUM_TIMERS];  // Array to store timer IDs

// Callback function for each timer
void timer_handler(int sig, siginfo_t *si, void *uc) {
    // Determine which timer triggered the signal
    timer_t *tidp = si->si_value.sival_ptr;

    if (tidp == &timers[0]) {
        // Handle timer 1
        printf("Timer 1 triggered\n");
    } 
    
    if (tidp == &timers[1]) {
        // Handle timer 2
        printf("Timer 2 triggered\n");
    } 
    
    if (tidp == &timers[2]) {
        // Handle timer 3
        printf("Timer 3 triggered\n");
    }
}

int main() {
    struct sigaction sa;
    struct sigevent sev;
    struct itimerspec its;

    // Set up signal handler
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);

    // Create and start multiple timers
    for (int i = 0; i < NUM_TIMERS; ++i) {
        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo = SIGALRM;
        sev.sigev_value.sival_ptr = &timers[i];
        timer_create(CLOCK_REALTIME, &sev, &timers[i]);

        // Configure timer to fire every 2 seconds
        its.it_value.tv_sec = i + 1;
        its.it_value.tv_nsec = 0;
        its.it_interval.tv_sec = i + 1;
        its.it_interval.tv_nsec = 0;

        timer_settime(timers[i], 0, &its, NULL);
    }

    // Wait indefinitely
    while (1) {
        sleep(1);  // Sleep or perform other tasks
    }

    return 0;
}