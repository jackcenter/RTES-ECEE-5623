#define _GNU_SOURCE

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

size_t count = 0;
pthread_mutex_t count_mutex;

void* service_0(void* threadp);
void* service_1(void* threadp);
void* service_2(void* threadp);
void* service_3(void* threadp);

typedef struct
{
  int first_idx;
  int last_idx;
} threadParams_t;

typedef struct
{
  size_t value;
  bool prime;
  bool claimed;
} prime_t;



int main() {
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

  const int number_of_threads = 4;

  pthread_attr_t pthread_attributes[number_of_threads];
  struct sched_param schedule_params[number_of_threads];
  pthread_t pthreads[number_of_threads];

  void* (*service_functions[number_of_threads])(void*);
  service_functions[0] = service_0;
  service_functions[1] = service_1;
  service_functions[2] = service_2;
  service_functions[3] = service_3;

  // Set each thread's first value to correspond to its group
  threadParams_t params[number_of_threads];
  params[0].first_idx = 2;
  params[0].last_idx = 100;
  params[1].first_idx = 101;
  params[1].last_idx = 200;
  params[2].first_idx = 201;
  params[2].last_idx = 300;
  params[3].first_idx =301;

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
  
  for (int i = 0; i < number_of_threads; ++i) {
       pthread_join(pthreads[i], NULL);
  }

  printf("Count: %d\n", count);
  return 0;
}



void* service_0(void* threadp)
{
  prime_t prime_array[98];
  for (size_t i = 2; i <= 100; ++i)
  {
    prime_t prime;
    prime.value = i;
    prime.prime = false;
    prime.claimed = false;
    prime_array[i-2] = prime; 
  }

  for (size_t i = 0; i < 98; ++i) {

    if (prime_array[i].claimed) {
      continue;
    }

    prime_array[i].claimed = true;
    prime_array[i].prime = true;
    int n = prime_array[i].value;

    printf("%d from Service 0\n", n);
    pthread_mutex_lock(&count_mutex);
    ++count;
    pthread_mutex_unlock(&count_mutex);

    for (int j = i+1; j < 98; ++j){
      if (prime_array[j].value % n == 0) {
        prime_array[j].claimed = true;
        prime_array[i].prime = true;
      }
    }
  }
}

void* service_1(void* threadp)
{
  prime_t prime_array[198];
  for (size_t i = 2; i <= 200; ++i)
  {
    prime_t prime;
    prime.value = i;
    prime.prime = false;
    prime.claimed = false;
    prime_array[i-2] = prime; 
  }

  for (size_t i = 0; i < 198; ++i) {

    if (prime_array[i].claimed) {
      continue;
    }

    prime_array[i].claimed = true;
    prime_array[i].prime = true;
    int n = prime_array[i].value;

    if (100 < n) {
      printf("%d from Service 1\n", n);
      pthread_mutex_lock(&count_mutex);
      ++count;
      pthread_mutex_unlock(&count_mutex);
    }

    for (int j = i+1; j < 198; ++j){
      if (prime_array[j].value % n == 0) {
        prime_array[j].claimed = true;
        prime_array[i].prime = true;
      }
    }
  }
}

void* service_2(void* threadp)
{
  prime_t prime_array[298];
  for (size_t i = 2; i <= 300; ++i)
  {
    prime_t prime;
    prime.value = i;
    prime.prime = false;
    prime.claimed = false;
    prime_array[i-2] = prime; 
  }

  for (size_t i = 0; i < 298; ++i) {

    if (prime_array[i].claimed) {
      continue;
    }

    prime_array[i].claimed = true;
    prime_array[i].prime = true;
    int n = prime_array[i].value;

    if (200 < n) {
      printf("%d from Service 2\n", n);
      pthread_mutex_lock(&count_mutex);
      ++count;
      pthread_mutex_unlock(&count_mutex);
    }

    for (int j = i+1; j < 298; ++j){
      if (prime_array[j].value % n == 0) {
        prime_array[j].claimed = true;
        prime_array[i].prime = true;
      }
    }
  }
}

void* service_3(void* threadp)
{
  prime_t prime_array[398];
  for (size_t i = 2; i <= 400; ++i)
  {
    prime_t prime;
    prime.value = i;
    prime.prime = false;
    prime.claimed = false;
    prime_array[i-2] = prime; 
  }

  for (size_t i = 0; i < 398; ++i) {

    if (prime_array[i].claimed) {
      continue;
    }

    prime_array[i].claimed = true;
    prime_array[i].prime = true;
    int n = prime_array[i].value;

    if (300 < n) {
      printf("%d from Service 3\n", n);
      pthread_mutex_lock(&count_mutex);
      ++count;
      pthread_mutex_unlock(&count_mutex);
    }

    for (int j = i+1; j < 398; ++j){
      if (prime_array[j].value % n == 0) {
        prime_array[j].claimed = true;
        prime_array[i].prime = true;
      }
    }
  }
}