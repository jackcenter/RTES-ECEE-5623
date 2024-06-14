#define _GNU_SOURCE   // for the CPU commands
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include <stdatomic.h>

#define COUNT  1000
#define NUM_CPUS 8
#define SCHED_POLICY SCHED_FIFO

pthread_mutex_t count_mutex;

void *threadOne(void *)
{
  pthread_mutex_lock(&count_mutex);
  struct timespec rtclk_time = {0, 0};
  clock_gettime(CLOCK_REALTIME, &rtclk_time);
  double current_time_s = (double)rtclk_time.tv_sec + ((double)rtclk_time.tv_nsec/1000000000.0);
  syslog(LOG_CRIT, "Thread One Start on CPU: %d at: %lf\n", sched_getcpu(), current_time_s);
    
  const double work_time_s = 0.01;
  const double end_time_s = current_time_s + work_time_s;
  do 
  {
    clock_gettime(CLOCK_REALTIME, &rtclk_time);
    current_time_s = (double)rtclk_time.tv_sec + ((double)rtclk_time.tv_nsec/1000000000.0);
  } 
  while (current_time_s < end_time_s);

  syslog(LOG_CRIT, "Thread One Stop on CPU: %d at: %lf\n", sched_getcpu(), current_time_s);
  pthread_mutex_unlock(&count_mutex);
  return (void *)0;
}


void *threadTwo(void *)
{
  struct timespec rtclk_time = {0, 0};
  clock_gettime(CLOCK_REALTIME, &rtclk_time);
  double current_time_s = (double)rtclk_time.tv_sec + ((double)rtclk_time.tv_nsec/1000000000.0);
  syslog(LOG_CRIT, "Thread Two Start on CPU: %d at: %lf\n", sched_getcpu(), current_time_s);

  const double work_time_s = 0.019;

  struct timespec intermediate_delay; 
  intermediate_delay.tv_sec = 0;
  intermediate_delay.tv_nsec = 1000000;  // 1ms
  double elapsed_time_s = 0.0;
  do 
  {
    nanosleep(&intermediate_delay, NULL);
    pthread_mutex_lock(&count_mutex);
    elapsed_time_s += 0.001;
    printf("%lf\n",elapsed_time_s);
    pthread_mutex_unlock(&count_mutex);
  } 
  while (elapsed_time_s < work_time_s);

  clock_gettime(CLOCK_REALTIME, &rtclk_time);
  current_time_s = (double)rtclk_time.tv_sec + ((double)rtclk_time.tv_nsec/1000000000.0);
  syslog(LOG_CRIT, "Thread Two Stop on CPU: %d at: %lf\n", sched_getcpu(), current_time_s);
  return (void *)0;
}

int main (int argc, char *argv[])
{
  // Set the main process to CPU 1
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);   // clear the cpu_set
  int cpu_idx = 1;
  CPU_SET(cpu_idx, &cpu_set);   // add cpu_idx to the cpu_set

  sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpu_set);

  const int max_prio=sched_get_priority_max(SCHED_POLICY);
  const int min_prio=sched_get_priority_min(SCHED_POLICY);

  // Set the main process to the lowest priority
  struct sched_param main_param;
  main_param.sched_priority=max_prio;
  if((sched_setscheduler(getpid(), SCHED_POLICY, &main_param)) < 0) {
        perror("sched_setscheduler");
  }

  // Move the threads to a different CPU
  CPU_ZERO(&cpu_set);   // clear the cpu_set
  cpu_idx = 2;
  CPU_SET(cpu_idx, &cpu_set);   // add cpu_idx to the cpu_set

  // Set up thread one
  pthread_t thread_one;

  pthread_attr_t thread_one_attr;
  pthread_attr_init(&thread_one_attr);
  pthread_attr_setschedpolicy(&thread_one_attr, SCHED_POLICY);
  pthread_attr_setaffinity_np(&thread_one_attr, sizeof(cpu_set_t), &cpu_set);

  struct sched_param thread_one_param;
  thread_one_param.sched_priority = max_prio;
  pthread_attr_setschedparam(&thread_one_attr, &thread_one_param);

  // Set up thread two 
  pthread_t thread_two;

  pthread_attr_t thread_two_attr;
  pthread_attr_init(&thread_two_attr);
  pthread_attr_setschedpolicy(&thread_two_attr, SCHED_POLICY);
  pthread_attr_setaffinity_np(&thread_two_attr, sizeof(cpu_set_t), &cpu_set);

  struct sched_param thread_two_param;
  thread_two_param.sched_priority = min_prio;
  pthread_attr_setschedparam(&thread_two_attr, &thread_two_param);

  // Set up "scheduler"
  struct timespec rtclk_time = {0, 0};
  clock_gettime(CLOCK_REALTIME, &rtclk_time);
  double current_time_s = (double)rtclk_time.tv_sec + ((double)rtclk_time.tv_nsec/1000000000.0);

  const double work_time_s = 0.1;
  const double end_time_s = current_time_s + work_time_s;

  bool is_thread_one_requested = true;
  const double thread_one_request_period_s = 0.02;
  double thread_one_request_time_s = current_time_s + thread_one_request_period_s;

  bool is_thread_two_requested = true;
  const double thread_two_request_period_s = 0.05;
  double thread_two_request_time_s = current_time_s + thread_two_request_period_s;

  syslog(LOG_CRIT, "Main Thread Start on CPU=%d\n", sched_getcpu());
  do
  {
    if (is_thread_one_requested)
    {
      pthread_create(&thread_one, &thread_one_attr, threadOne, NULL);
      is_thread_one_requested = false;
    }

    // Check if it's time to request thread one again
    if (thread_one_request_time_s < current_time_s)
    {
      thread_one_request_time_s += thread_one_request_period_s;
      is_thread_one_requested = true;
    }

    if (is_thread_two_requested)
    {
      pthread_create(&thread_two, &thread_two_attr, threadTwo, NULL);
      is_thread_two_requested = false;
    }

    // Check if it's time to request thread two again
    if (thread_two_request_time_s < current_time_s)
    {
      thread_two_request_time_s += thread_two_request_period_s;
      is_thread_two_requested = true;
    }

    clock_gettime(CLOCK_REALTIME, &rtclk_time);
    current_time_s = (double)rtclk_time.tv_sec + ((double)rtclk_time.tv_nsec/1000000000.0);
  } 
  while (current_time_s < end_time_s);

  pthread_join(thread_one, NULL);
  pthread_join(thread_two, NULL);
  printf("TEST COMPLETE\n");
}
