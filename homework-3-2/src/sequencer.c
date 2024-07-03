// This is necessary for CPU affinity macros in Linux
#define _GNU_SOURCE

#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <types.h>

sem_t sem_0;
sem_t sem_1;

pthread_mutex_t mutex_0;

static timer_t timers[2];


bool abort_service_0 = false;
bool abort_service_1 = false;

volatile State state = {
  .position.latitude = 0.0,
  .position.longitude = 0.0,
  .position.altitude = 0.0,
  .attitude.roll = 0.0,
  .attitude.pitch = 0.0,
  .attitude.yaw = 0.0,
  .timestamp_s = 0.0
};

void interval_expired(int signum, siginfo_t *info, void *context);
// void interval_expired_1(int signum, siginfo_t *info, void *context);

void* service_0(void* threadp);
void* service_1(void* threadp);

void setup_main_thread(void);

void main(void)
{
  setup_main_thread();

  const int rt_max_prio = sched_get_priority_max(SCHED_FIFO);

  cpu_set_t thread_cpu_set;
  CPU_ZERO(&thread_cpu_set);
  CPU_SET(3, &thread_cpu_set);

  const int number_of_threads = 2;
  pthread_attr_t pthread_attributes[number_of_threads];
  struct sched_param schedule_params[number_of_threads];
  pthread_t pthreads[number_of_threads];

  void* (*service_functions[number_of_threads])(void*);
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

  {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;             
    sa.sa_sigaction = interval_expired;   // signal callback function
    sigemptyset(&sa.sa_mask);             // clear the signal mask
    sigaction(SIGRTMIN, &sa, NULL);        // action to take on SIGALRM

    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timers[0];
    timer_create(CLOCK_REALTIME, &sev, &timers[0]);

    struct itimerspec its;
    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 1;
    its.it_interval.tv_nsec = 0;
    timer_settime(timers[0], 0, &its, NULL);
  }

  // {
  //   struct sigaction sa;
  //   sa.sa_flags = SA_SIGINFO;             
  //   sa.sa_sigaction = interval_expired_1;   // signal callback function
  //   sigemptyset(&sa.sa_mask);             // clear the signal mask
  //   sigaction(SIGRTMIN + 1, &sa, NULL);        // action to take on SIGALRM

  //   struct sigevent sev;
  //   sev.sigev_notify = SIGEV_SIGNAL;
  //   sev.sigev_signo = SIGRTMIN + 1;
  //   sev.sigev_value.sival_ptr = &timers[0];
  //   timer_create(CLOCK_REALTIME, &sev, &timers[0]);

  //   struct itimerspec its;
  //   its.it_value.tv_sec = 2;
  //   its.it_value.tv_nsec = 0;
  //   its.it_interval.tv_sec = 2;
  //   its.it_interval.tv_nsec = 0;
  //   timer_settime(timers[0], 0, &its, NULL); 
  // }


  // sev.sigev_notify = SIGEV_SIGNAL;
  // sev.sigev_signo = SIGALRM;
  // sev.sigev_value.sival_ptr = &timers[1];
  // timer_create(CLOCK_REALTIME, &sev, &timers[1]);

  // its.it_value.tv_sec = 2;
  // its.it_value.tv_nsec = 0;
  // its.it_interval.tv_sec = 2;
  // its.it_interval.tv_nsec = 0;
  // timer_settime(timers[1], 0, &its, NULL);

  printf("Main: %d\n", getpid());
  for(;;) {
    pause();
  }

  for(int i = 0; i < number_of_threads; ++i) {
    pthread_join(pthreads[i], NULL);
  }

  printf("Done\n");
}

size_t count = 0;
void interval_expired(int signum, siginfo_t *info, void *context) {
  
  printf("Post 0: %d\n", signum);
  sem_post(&sem_0);

  if (++count % 10 == 0) {
    sem_post(&sem_1);
  }
}

// void interval_expired_1(int signum, siginfo_t *info, void *context) {
//   // printf("Post 1: %d\n", signum);
//   // sem_post(&sem_1);
// }


void setup_main_thread(void) {
  if (sem_init (&sem_0, 0, 0)) { 
    printf("Failed to initialize S0 semaphore\n");
    exit (-1);
  }

  if (sem_init (&sem_1, 0, 0)) { 
    printf("Failed to initialize S1 semaphore\n");
    exit (-1);
  }

  const pid_t main_pid=getpid();

  struct sched_param main_param;
  if (sched_getparam(main_pid, &main_param)) {
    printf("Failed to get scheduling parameters for `main_param`.\n");
    exit(-1);
  }

  main_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
  if (sched_setscheduler(main_pid, SCHED_FIFO, &main_param)) {
    printf("Failed to set scheduling parameters for `main_param`. Do you use sudo?\n");
    exit(-1);
  }
}

void* service_0(void* threadp)
{
  struct timespec start_timespec = {0, 0};
  clock_gettime(CLOCK_REALTIME, &start_timespec);
  double start_time_s = (double)start_timespec.tv_sec + (double)start_timespec.tv_nsec / 1000000000.0f;

  while(!abort_service_0) {
    sem_wait(&sem_0);

    struct timespec current_timespec = {0, 0};
    clock_gettime(CLOCK_REALTIME, &current_timespec);

    const double time_s = (double)current_timespec.tv_sec + (double)current_timespec.tv_nsec / 1000000000.0f;
    const double timestamp_s = time_s - start_time_s;

    const State new_state = {
      .position.latitude = 0.01 * timestamp_s,
      .position.longitude = 0.2 * timestamp_s,
      .position.altitude = 0.25 * timestamp_s,
      .attitude.roll = sin(timestamp_s),
      .attitude.pitch = cos(timestamp_s * timestamp_s),
      .attitude.yaw = cos(timestamp_s),
      .timestamp_s = timestamp_s
    };
    printf("Write timestamp: %.2lf s\n", new_state.timestamp_s);

    if((pthread_mutex_lock(&mutex_0)))
    {
        printf("Service 0 mutex lock errror. Exiting thread.\n");
        pthread_exit(NULL);
    }
    state = new_state;

    pthread_mutex_unlock(&mutex_0);
  }

  pthread_exit((void*) 0);
}

void* service_1(void* threadp)
{
  while(!abort_service_1) {
    sem_wait(&sem_1);

    if((pthread_mutex_lock(&mutex_0)))
    {
        printf("Service 1 mutex lock errror. Exiting thread.\n");
        pthread_exit(NULL);
    }

    const State read_state = state;

    pthread_mutex_unlock(&mutex_0);


    printf("State: %.3lf, %.3lf, %.3lf, %.3lf, %.3lf, %.3lf, %.3lf\n", state.position.latitude, state.position.longitude,
           state.position.altitude, state.attitude.roll, state.attitude.pitch, state.attitude.yaw, state.timestamp_s);
  }

  pthread_exit((void*) 0);
}