#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include <types.h>

void* service_0(void* threadp);
void* service_1(void* threadp);
void* service_2(void* threadp);

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

  // Set up the threads
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
  
  // Create the threads
  for (int i = 0; i < number_of_threads; ++i) {
    if (pthread_create(&pthreads[i], NULL, service_functions[i], (void*)&(params[i]))) {
      printf("Failed to create `pthreads` for thread: %d.\n", 0);
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

  thread_params->sum = sum;
  printf("Sum from idx: %d to %d equals: %d\n", thread_params->first_idx, (thread_params->first_idx + 99), sum);
}
