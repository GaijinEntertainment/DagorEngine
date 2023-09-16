#include <perfMon/dag_cpuFreq.h>
#include <time.h>

static int64_t startup_ref_time = 0;
static bool default_measure_cpu_freq()
{
  startup_ref_time = ref_time_ticks();
  return false;
}

static bool measured = default_measure_cpu_freq();

void measure_cpu_freq(bool)
{
  if (measured)
    return;
  measured = true;
  startup_ref_time = ref_time_ticks();
}

int64_t ref_time_ticks() { return clock_gettime_nsec_np(CLOCK_UPTIME_RAW); }

int64_t ref_ticks_frequency() { return 1000000000; }

int64_t rel_ref_time_ticks(int64_t ref, int time_usec) { return ref + time_usec * 1000; }

int64_t ref_time_delta_to_nsec(int64_t ref) { return ref; }
int64_t ref_time_delta_to_usec(int64_t ref) { return ref / 1000; }

int64_t get_time_nsec(int64_t ref) { return ref_time_delta_to_nsec(ref_time_ticks() - ref); }
int get_time_usec(int64_t ref) { return (int)ref_time_delta_to_usec(ref_time_ticks() - ref); }

int get_time_msec() { return int((ref_time_ticks() - startup_ref_time) / int64_t(1000000)); }
