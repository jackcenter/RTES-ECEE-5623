#define _GNU_SOURCE     // for the CPU commands
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <semaphore.h>
#include <unistd.h>

#include <stdatomic.h>

#define COUNT  100
#define NUM_CPUS 8

typedef struct
{
    int threadIdx;
} threadParams_t;


// Unsafe global
int gsum=0;

// Safer ATOMIC global
atomic_int agsum=0;

sem_t semaphore;

void *incThread(void *threadp)
{
  // Wait until the semaphore is available and claim it.
  sem_wait(&semaphore);
  threadParams_t *threadParams = (threadParams_t *)threadp;

  for(int i=0; i<COUNT; ++i)
  {
      gsum=gsum+i;
      agsum=agsum+i;
      printf("Increment thread idx=%d, gsum=%d,  agsum=%d, CPU=%d\n", threadParams->threadIdx, gsum, agsum, sched_getcpu());
  }
  // Release the semaphore
  sem_post(&semaphore);
  return (void *)0;
}


void *decThread(void *threadp)
{
  // Wait until the semaphore is available and claim it.
  sem_wait(&semaphore);
  threadParams_t *threadParams = (threadParams_t *)threadp;

  for(int i=0; i<COUNT; ++i)
  {
      gsum=gsum-i;
      agsum=agsum-i;
      printf("Decrement thread idx=%d, gsum=%d, agsum=%d, CPU=%d\n", threadParams->threadIdx, gsum, agsum, sched_getcpu());
  }
  // Release the semaphore
  sem_post(&semaphore);
  return (void *)0;
}

int main (int argc, char *argv[])
{
  // Initialize the semaphore as ready
  if(sem_init(&semaphore, 0, 1) != 0) {
    printf ("Failed to initialize semaphore\n");
    exit (-1);
  }

  // Set everything to run on the same CPU
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  int cpu_idx = 3;
  CPU_SET(cpu_idx, &cpu_set);

  // Set the main process
  sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpu_set);

  pthread_attr_t fifo_sched_attr;
  pthread_attr_init(&fifo_sched_attr);
  pthread_attr_setaffinity_np(&fifo_sched_attr, sizeof(cpu_set_t), &cpu_set);

  printf("main thread running on CPU=%d\n", sched_getcpu());

  // Handle the increment thread
  pthread_t increment_thread;


  threadParams_t inc_thread_params;
  inc_thread_params.threadIdx = 0;

  pthread_create(&increment_thread, &fifo_sched_attr, incThread, (void *)&inc_thread_params);

  // Handle the decrement thread
  pthread_t decrement_thread;

  threadParams_t dec_thread_params;
  dec_thread_params.threadIdx = 1;

  pthread_create(&decrement_thread, &fifo_sched_attr, decThread, (void *)&dec_thread_params);

  // Clean up
  pthread_join(increment_thread, NULL);
  pthread_join(decrement_thread, NULL);
  printf("TEST COMPLETE\n");
}
