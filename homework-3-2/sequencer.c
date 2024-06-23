// This is necessary for CPU affinity macros in Linux
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <time.h>

sem_t sem_0;
sem_t sem_1;

void* service_0(void* threadp);
void* service_1(void* threadp);

void main(void)
{
  if (sem_init (&sem_0, 0, 0)) { 
    printf("Failed to initialize S1 semaphore\n");
    exit (-1);
  }

  if (sem_init (&sem_1, 0, 0)) { 
    printf("Failed to initialize S1 semaphore\n");
    exit (-1);
  }

  const pid_t main_pid=getpid();

  const int rt_max_prio = sched_get_priority_max(SCHED_FIFO);
  const int rt_min_prio = sched_get_priority_min(SCHED_FIFO);

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

  cpu_set_t thread_cpu_set;
  CPU_ZERO(&thread_cpu_set);
  CPU_SET(3, &thread_cpu_set);

  const int number_of_threads = 2;
  pthread_attr_t pthread_attributes[number_of_threads];
  struct sched_param schedule_params[number_of_threads];
  pthread_t pthreads[number_of_threads];

  void* (*service_functions[2])(void*);
  service_functions[0] = service_0;
  service_functions[1] = service_1;

  for(int i = 0; i < number_of_threads; ++i)
  {
    
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

    schedule_params[i].sched_priority=rt_max_prio - i;
    if (pthread_attr_setschedparam(&pthread_attributes[i], &schedule_params[i])) {
      printf("Failed to set sched param for `pthread_attrubutes` for thread: %d.\n", i);
      exit(-1);
    }

    if (pthread_create(&pthreads[i], &pthread_attributes[i], service_functions[i], NULL)) {
      printf("Failed to create `pthreads` for thread: %d.\n", i);
      exit(-1);
    }
  }

  sem_post(&sem_0);
  sem_post(&sem_1);

  for(int i = 0; i < number_of_threads; ++i) {
    pthread_join(pthreads[i], NULL);
  }

  printf("Done\n");
}

void* service_0(void* threadp)
{
    // struct timeval current_time_val;
    // double current_time;
    // unsigned long long S1Cnt=0;
    // threadParams_t *threadParams = (threadParams_t *)threadp;

    // gettimeofday(&current_time_val, (struct timezone *)0);
    // syslog(LOG_CRIT, "Frame Sampler thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    // printf("Frame Sampler thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    // while(!abortS1)
    // {
    //     sem_wait(&semS1);
    //     S1Cnt++;

    //     gettimeofday(&current_time_val, (struct timezone *)0);
    //     syslog(LOG_CRIT, "Frame Sampler release %llu @ sec=%d, msec=%d\n", S1Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    // }
  sem_wait(&sem_0);
  printf("Service 0\n");
  pthread_exit((void*) 0);
}

void* service_1(void* threadp)
{
    // struct timeval current_time_val;
    // double current_time;
    // unsigned long long S1Cnt=0;
    // threadParams_t *threadParams = (threadParams_t *)threadp;

    // gettimeofday(&current_time_val, (struct timezone *)0);
    // syslog(LOG_CRIT, "Frame Sampler thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    // printf("Frame Sampler thread @ sec=%d, msec=%d\n", (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);

    // while(!abortS1)
    // {
    //     sem_wait(&semS1);
    //     S1Cnt++;

    //     gettimeofday(&current_time_val, (struct timezone *)0);
    //     syslog(LOG_CRIT, "Frame Sampler release %llu @ sec=%d, msec=%d\n", S1Cnt, (int)(current_time_val.tv_sec-start_time_val.tv_sec), (int)current_time_val.tv_usec/USEC_PER_MSEC);
    // }
  sem_wait(&sem_1);
  printf("Service 1\n");
  pthread_exit((void*) 0);
}