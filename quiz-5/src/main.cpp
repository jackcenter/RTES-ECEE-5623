// This is necessary for CPU affinity macros in Linux
// #define _GNU_SOURCE

#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

typedef void (*TimerCallback)(int, siginfo_t*, void*);

sem_t sem_0;
sem_t sem_1;
sem_t sem_2;

static timer_t timers[3];

bool abort_service_0 = false;
bool abort_service_1 = false;
bool abort_service_2 = false;

void sigint_handler(int sig);
void interval_expired(int signum, siginfo_t* info, void* context);

void* service_0(void* threadp);
void* service_1(void* threadp);
void* service_2(void* threadp);

static void setup_main_thread(void);
static void initialize_timer(const struct timespec period, const size_t id, TimerCallback callback,
                             timer_t* const timer);

static double getCurrentTime();

int main(void) {
  signal(SIGINT, sigint_handler);   // handles exiting by ctrl-c
  setup_main_thread();
  
  const int rt_max_prio = sched_get_priority_max(SCHED_FIFO);

  cpu_set_t thread_cpu_set;
  CPU_ZERO(&thread_cpu_set);
  CPU_SET(3, &thread_cpu_set);

  const int number_of_threads = 3;
  pthread_attr_t pthread_attributes[number_of_threads];
  struct sched_param schedule_params[number_of_threads];
  pthread_t pthreads[number_of_threads];

  void* (*service_functions[number_of_threads])(void*);
  service_functions[0] = service_0;
  service_functions[1] = service_1;
  service_functions[2] = service_2;

  for (int i = 0; i < number_of_threads; ++i) {
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

    schedule_params[i].sched_priority = rt_max_prio - i;
    if (pthread_attr_setschedparam(&pthread_attributes[i], &schedule_params[i])) {
      printf("Failed to set sched param for `pthread_attrubutes` for thread: %d.\n", i);
      exit(-1);
    }

    if (pthread_create(&pthreads[i], &pthread_attributes[i], service_functions[i], NULL)) {
      printf("Failed to create `pthreads` for thread: %d.\n", i);
      exit(-1);
    }
  }

  struct timespec timer_0_period = {0, 033333333};
  initialize_timer(timer_0_period, 0, interval_expired, &timers[0]);

  while (!abort_service_0 && !abort_service_1 && !abort_service_2) {
    pause();
  }

  abort_service_0 = true;
  abort_service_1 = true;
  abort_service_2 = true;

  for (size_t i = 0; i < number_of_threads; ++i) {
    pthread_join(pthreads[i], 0);
  }

  return 0;
}

void interval_expired(int signum, siginfo_t* info, void* context) {
  static size_t count = 0;
  ++count;
  sem_post(&sem_0);

  if (count % 2 == 0) {
    sem_post(&sem_1);
  }

  if (count % 3 == 0) {
    sem_post(&sem_2);
  }

  if (count % 30 == 0) {
    abort_service_0 = true;
    abort_service_1 = true;
    abort_service_2 = true;
  }
}

void setup_main_thread(void) {
  if (sem_init(&sem_0, 0, 0)) {
    printf("Failed to initialize S0 semaphore\n");
    exit(-1);
  }

  if (sem_init(&sem_1, 0, 0)) {
    printf("Failed to initialize S1 semaphore\n");
    exit(-1);
  }

  if (sem_init(&sem_2, 0, 1)) {
    printf("Failed to initialize S2 semaphore\n");
    exit(-1);
  }

  const pid_t main_pid = getpid();

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

void initialize_timer(const struct timespec period, const size_t id, TimerCallback callback, timer_t* const timer) {
  struct sigaction sa;
  sa.sa_flags = SA_SIGINFO;        // expect additional info
  sa.sa_sigaction = callback;      // signal callback function
  sigemptyset(&sa.sa_mask);        // clear the signal mask
  sigaction(SIGRTMIN, &sa, NULL);  // action to take on SIGALRM

  struct sigevent sev;
  sev.sigev_notify = SIGEV_SIGNAL + id;  // send a signal
  sev.sigev_signo = SIGRTMIN;            // signal number
  sev.sigev_value.sival_ptr = timer;
  timer_create(CLOCK_REALTIME, &sev, timer);

  struct itimerspec its;
  its.it_value.tv_sec = period.tv_sec;
  its.it_value.tv_nsec = period.tv_nsec;
  its.it_interval.tv_sec = period.tv_sec;
  its.it_interval.tv_nsec = period.tv_nsec;
  timer_settime(*timer, 0, &its, NULL);
}

void sigint_handler(int sig) {
  abort_service_0 = true;
  abort_service_1 = true;
  abort_service_2 = true;
}


void* service_0(void* threadp) {
  struct timespec start_timespec = {0, 0};
  clock_gettime(CLOCK_REALTIME, &start_timespec);
  double start_time_s = (double)start_timespec.tv_sec + (double)start_timespec.tv_nsec / 1000000000.0f;

  while (!abort_service_0) {
    sem_wait(&sem_0);
    syslog(LOG_CRIT, "QUIZ-5: Service 0 at %.3lf", getCurrentTime());
  }

  pthread_exit((void*)0);
}

void* service_1(void* threadp) {
  while (!abort_service_1) {
    sem_wait(&sem_1);
    syslog(LOG_CRIT, "QUIZ-5: Service 1 at %.3lf", getCurrentTime());
  }

  pthread_exit((void*)0);
}

void* service_2(void* threadp) {
  while (!abort_service_2) {
    sem_wait(&sem_2);
    syslog(LOG_CRIT, "QUIZ-5: Service 2at %.3lf", getCurrentTime());
  }

  pthread_exit((void*)0);
}

static double getCurrentTime() {
  struct timespec current_timespec = {0, 0};
  clock_gettime(CLOCK_REALTIME, &current_timespec);
  return ((double)current_timespec.tv_sec + (double)current_timespec.tv_nsec / 1000000000.0f);
}
