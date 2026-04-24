#include <perfMon/dag_cpuFreq.h>

namespace rtc
{

int64_t SystemTimeNanos() { return get_time_nsec(ref_time_ticks()); }

} // namespace rtc
