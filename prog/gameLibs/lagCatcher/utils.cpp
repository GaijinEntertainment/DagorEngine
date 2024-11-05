// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "utils.h"
#include <string.h>

int create_timer(int signo, timer_t *timer, void *arg)
{
  struct sigevent sevp;
  memset(&sevp, 0, sizeof(sevp));
  sevp.sigev_notify = SIGEV_SIGNAL;
  sevp.sigev_signo = signo;
  sevp.sigev_value.sival_ptr = arg;
  return timer_create(CLOCK_MONOTONIC, &sevp, timer);
}

int destroy_timer(timer_t *timer)
{
  int ret = timer_delete(*timer);
  *timer = NULL;
  return ret;
}

int start_timer(timer_t *timer, int time_us, bool once)
{
  struct itimerspec its;
  memset(&its, 0, sizeof(its));
  its.it_value.tv_nsec = time_us * 1000;
  if (!once)
    its.it_interval = its.it_value;
  return timer_settime(*timer, 0, &its, NULL);
}

int stop_timer(timer_t *timer)
{
  struct itimerspec its;
  memset(&its, 0, sizeof(its));
  return timer_settime(*timer, 0, &its, NULL);
}

void enable_signal_handler(int signo, void (*handler)(int, siginfo_t *, void *))
{
  struct sigaction sa;
  if (handler)
  {
    sa.sa_sigaction = handler;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
  }
  else
  {
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = SA_RESTART;
  }
  sigemptyset(&sa.sa_mask);
  sigaction(signo, &sa, NULL);
}

void disable_signal_handler(int signo) { enable_signal_handler(signo, NULL); }

void block_signals(int *signals, bool block, sigset_t *old_set)
{
  sigset_t set;
  sigemptyset(&set);
  for (int i = 0; signals[i]; ++i)
    sigaddset(&set, signals[i]);
  pthread_sigmask(block ? SIG_BLOCK : SIG_UNBLOCK, &set, old_set);
}
