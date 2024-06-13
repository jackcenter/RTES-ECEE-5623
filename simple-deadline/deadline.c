// https://www.i-programmer.info/programming/cc/13002-applying-c-deadline-scheduling.html?start=1
//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <pthread.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/syscall.h>

struct sched_attr 
{
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    int32_t sched_nice;
    uint32_t sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};

int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags) 
{
    return syscall(__NR_sched_setattr, pid, attr, flags);
}

void * threadA(void *p) 
{
    const uint32_t policy = *(uint32_t *)p;
    struct sched_attr attr = 
    {
        .size = sizeof (attr),
        .sched_policy = policy,
        .sched_runtime = 10 * 1000 * 1000,
        .sched_period = 2 * 1000 * 1000 * 1000,
        .sched_deadline = 11 * 1000 * 1000
    };

    sched_setattr(0, &attr, 0);

    struct timespec rtclk_time = {0, 0};
    for (;;) 
    {   
        clock_gettime(CLOCK_REALTIME, &rtclk_time);
        const double current_time = (double)rtclk_time.tv_sec + ((double)rtclk_time.tv_nsec/1000000000.0);
        
        printf("Time is %lf\n", current_time);
        fflush(0);
        sched_yield();
    };
}

/**
* Use 0 for SCHED_FIFO and 1 for SCHED_DEADLINE
*/
int main(int argc, char** argv) 
{
    if (argc != 2) {
        printf("The number of arguments passed in, %d, must equal 1\n", argc);
        return 1;
    }

    uint32_t schedule_policy;
    const int input = atoi(argv[1]);
    if(input == 0) {
        schedule_policy = SCHED_FIFO;
    } else if (input == 1) {
        schedule_policy = SCHED_DEADLINE;
    } else
    {
        printf("The number of arguments passed must me `0` or `1`\n", argv);
    }

    pthread_t pthreadA;
    pthread_create(&pthreadA, NULL, threadA, (void *)&schedule_policy);
    pthread_exit(0);
    return (EXIT_SUCCESS);
}
