
#include "time.h"
#include <chrono>

static uint64_t micros()
{
    uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::
                  now().time_since_epoch()).count();
    return us;
}

double TimeKeeper::get_time()
{
  return micros();
}

TimeKeeper::~TimeKeeper()
{
//	std::cout << "~TimeKeeper(" << s << "): elapsed time " << get_relative_time() << " seconds.\n";
}
