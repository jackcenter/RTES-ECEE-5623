#define _GNU_SOURCE   // for the CPU commands
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>

#include <stdatomic.h>

#define COUNT  1000
#define NUM_CPUS 8
#define SCHED_POLICY SCHED_FIFO

typedef struct
{
    int threadIdx;
} threadParams_t;


// Unsafe global
int gsum=0;

// Safer ATOMIC global
atomic_int agsum=0;

void *incThread(void *threadp)
{
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(int i=0; i<COUNT; ++i)
    {
        gsum=gsum+i;
        agsum=agsum+i;
        printf("Increment thread idx=%d, gsum=%d,  agsum=%d, CPU=%d\n", threadParams->threadIdx, gsum, agsum, sched_getcpu());
    }
    return (void *)0;
}


void *decThread(void *threadp)
{
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(int i=0; i<COUNT; ++i)
    {
        gsum=gsum-i;
        agsum=agsum-i;
        printf("Decrement thread idx=%d, gsum=%d, agsum=%d, CPU=%d\n", threadParams->threadIdx, gsum, agsum, sched_getcpu());
    }
    return (void *)0;
}

int main (int argc, char *argv[])
{
  // Set everything to run on the same CPU
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);   // clear the cpu_set
  int cpu_idx = 3;
  CPU_SET(cpu_idx, &cpu_set);   // add cpu_idx to the cpu_set

  // Set the main process to the cpu_set
  sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpu_set);

  // Initialize the pthread_attr with the schedule policy and to the cpu_set
  pthread_attr_t fifo_sched_attr;
  pthread_attr_init(&fifo_sched_attr);
  pthread_attr_setschedpolicy(&fifo_sched_attr, SCHED_POLICY);
  pthread_attr_setaffinity_np(&fifo_sched_attr, sizeof(cpu_set_t), &cpu_set);

  // Set the main process to the schedule process with the highest priority
  const int max_prio=sched_get_priority_max(SCHED_POLICY);
  struct sched_param fifo_param;
  fifo_param.sched_priority=max_prio;
  if((sched_setscheduler(getpid(), SCHED_POLICY, &fifo_param)) < 0) {
        perror("sched_setscheduler");
  }

  printf("main thread running on CPU=%d\n", sched_getcpu());

  // Handle the increment thread
  pthread_t increment_thread;

  // Set the increment thread to the second highest priority
  fifo_param.sched_priority = max_prio - 1;
  pthread_attr_setschedparam(&fifo_sched_attr, &fifo_param);

  threadParams_t inc_thread_params;
  inc_thread_params.threadIdx = 0;

  // Create the increment thread
  pthread_create(&increment_thread, &fifo_sched_attr, incThread, (void *)&inc_thread_params);

  // Handle the decrement thread
  pthread_t decrement_thread;

  // Set the increment thread to the lowest priority
  const int min_prio=sched_get_priority_min(SCHED_POLICY);
  fifo_param.sched_priority = min_prio;
  pthread_attr_setschedparam(&fifo_sched_attr, &fifo_param);

  threadParams_t dec_thread_params;
  dec_thread_params.threadIdx = 1;

  // Create the decrement thread
  pthread_create(&decrement_thread, &fifo_sched_attr, decThread, (void *)&dec_thread_params);

  // Clean up
  pthread_join(increment_thread, NULL);
  pthread_join(decrement_thread, NULL);
  printf("TEST COMPLETE\n");
}