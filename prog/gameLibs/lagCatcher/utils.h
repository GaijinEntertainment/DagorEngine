#pragma once

#include <signal.h>
#include <time.h>

int create_timer(int signo, timer_t *timer, void *arg);
int destroy_timer(timer_t *timer);
int start_timer(timer_t *timer, int time_us, bool once);
int stop_timer(timer_t *timer);

void enable_signal_handler(int signo, void (*handler)(int, siginfo_t *, void *));
void disable_signal_handler(int signo);

void block_signals(int *signals, bool block, sigset_t *old_set = NULL);
