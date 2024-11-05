// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if (__cplusplus >= 201100L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201100L)
#include "Timer.hpp"

Timer::Timer() : _current(std::chrono::high_resolution_clock::now()), _prev(_current), _start(_current){};
#endif
