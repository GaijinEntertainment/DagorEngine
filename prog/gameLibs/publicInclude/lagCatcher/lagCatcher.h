//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#ifdef __linux__
#include <signal.h>
#endif

#ifndef SIGUSR2
#define SIGUSR2 12
#endif

#define LAGCATCHER_DEFAULT_DEADLINE_SIGNAL SIGUSR2

namespace lagcatcher
{

// this is need to be called before any thread creation to make sure that only calling thread will be profiled
void init_early(int deadline_signal = LAGCATCHER_DEFAULT_DEADLINE_SIGNAL);

// Note: install 2 signal handles: for deadline_signal (SIGUSR2 by default) & SIGPROF
bool init(int sampling_interval_us = 10000, int mem_budget = 200 << 10, int deadline_signal = LAGCATCHER_DEFAULT_DEADLINE_SIGNAL);
void shutdown();

// start collect samples of calling thread if stop()/start() isn't called before deadline time (i.e. start timers)
// typically this need to be called once per frame
void start(int deadline_ms);
void stop();

// stop collecting samples & clear collected data
void clear();
// stop collecting samples & flush collected samples to text file
// return number of samples written
// Warning: perform backtrace resolution, which can be quite slow (5 ms for one sample!)
int flush(const char *log_path);

}; // namespace lagcatcher
