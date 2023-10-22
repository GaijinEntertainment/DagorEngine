#include "daProfilePlatform.h"
#include "daGpuProfiler.h"

namespace da_profiler
{
uint64_t gpu_frequency() { return gpu_profiler::ticks_per_second(); }
bool gpu_cpu_ticks_ref(int64_t &cpu, int64_t &gpu) { return gpu_profiler::get_profile_cpu_ticks_reference(cpu, gpu); }
} // namespace da_profiler
