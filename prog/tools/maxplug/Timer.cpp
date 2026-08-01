// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "Timer.hpp"

Timer::Timer() : _current(std::chrono::high_resolution_clock::now()), _prev(_current), _start(_current) {}
