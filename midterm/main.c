#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void* service_0(void* threadp);

typedef struct
{
  int first_idx;
  int sum;
} threadParams_t;

// Global list to hold numbers
int list_of_numbers[300];

void main(void)
{
  // Create the list of numbers in global scope
  for (int i = 0; i < 300; ++i)
  {
    list_of_numbers[i] = i; 
  }
  
  const int rt_max_prio = sched_get_priority_max(SCHED_FIFO);
  
  // Set main thread as highest priority
  const pid_t main_pid=getpid();

  struct sched_param main_param;
  if (sched_getparam(main_pid, &main_param)) {
    printf("Failed to get scheduling parameters for `main_param`.\n");
    exit(-1);
  }

  main_param.sched_priority = rt_max_prio;
  if (sched_setscheduler(main_pid, SCHED_FIFO, &main_param)) {
    printf("Failed to set scheduling parameters for `main_param`. Do you use sudo?\n");
    exit(-1);
  }

  // Create the threads
  cpu_set_t thread_cpu_set;
  CPU_ZERO(&thread_cpu_set);
  CPU_SET(3, &thread_cpu_set);

  const int number_of_threads = 3;

  pthread_attr_t pthread_attributes[number_of_threads];
  struct sched_param schedule_params[number_of_threads];
  pthread_t pthreads[number_of_threads];

  void* (*service_functions[number_of_threads])(void*);
  service_functions[0] = service_0;
  service_functions[1] = service_0;
  service_functions[2] = service_0;

  // Set each thread's first value to correspond to its group
  threadParams_t params[number_of_threads];
  params[0].first_idx = 0;
  params[0].sum = 0;
  params[1].first_idx = 100;
  params[1].sum = 0;
  params[2].first_idx = 200;
  params[2].sum = 0;

  for (size_t i = 0; i < number_of_threads; ++i) {
    if (pthread_attr_init(&pthread_attributes[i])) {
      printf("Failed to initialize `pthread_attrubutes` for thread: %d.\n", i);
      exit(-1);
    }

    if (pthread_attr_setinheritsched(&pthread_attributes[i], PTHREAD_EXPLICIT_SCHED)) {
      printf("Failed to set inherit schedule for `pthread_attrubutes` for thread: %d.\n", i);
      exit(-1);
    }

    if (pthread_attr_setschedpolicy(&pthread_attributes[i], SCHED_FIFO)) {
      printf("Failed to set sched policy for `pthread_attrubutes` for thread: %d.\n", i);
      exit(-1);
    }


    if (pthread_attr_setaffinity_np(&pthread_attributes[i], sizeof(cpu_set_t), &thread_cpu_set)) {
      printf("Failed to set affinity for `pthread_attrubutes` for thread: %d.\n", i);
      exit(-1);
    }

    schedule_params[i].sched_priority=rt_max_prio - i - 1;
    if (pthread_attr_setschedparam(&pthread_attributes[i], &schedule_params[i])) {
      printf("Failed to set sched param for `pthread_attrubutes` for thread: %d.\n", i);
      exit(-1);
    }

    if (pthread_create(&pthreads[i], &pthread_attributes[i], service_functions[i], (void*)&(params[i]))) {
      printf("Failed to create `pthreads` for thread: %d.\n", i);
      exit(-1);
    }
  }

  // Wait for threads to join
  for (int i = 0; i < number_of_threads; ++i) {
       pthread_join(pthreads[i], NULL);
  }
  
  int final_sum = 0;
  for (int i = 0; i < number_of_threads; ++i) {
    final_sum += params[i].sum;
  }
  
  printf("Done, final sum equals: %d\n", final_sum);
}

void* service_0(void* threadp)
{
  threadParams_t* thread_params = (threadParams_t*)threadp;
  int sum = 0;
  for (int i = thread_params->first_idx; i < (thread_params->first_idx + 100); ++i) {
    sum += list_of_numbers[i];
  }

  int policy;
  struct sched_param thread_schedp;
  pthread_getschedparam(pthread_self(), &policy, &thread_schedp);
  printf("thread prio=%d for thread starting at %d\n", thread_schedp.sched_priority, thread_params->first_idx);

  thread_params->sum = sum;
  printf("Sum from idx: %d to %d equals: %d\n", thread_params->first_idx, (thread_params->first_idx + 99), sum);
}
