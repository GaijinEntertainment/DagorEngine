// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_console.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_daProfilerSettings.h>
#include <util/dag_string.h>
using namespace console;

#if TIME_PROFILER_ENABLED

static inline void show_current_modes()
{
  console::print_d("current profiler settings: ");
  uint32_t mode = da_profiler::get_current_mode();
  bool first = true;
  for (int pi = 0, pe = da_profiler::profiler_modes_count(); pi < pe; ++pi)
  {
    uint32_t m = da_profiler::profiler_mode_index(pi);
    if (!m)
      break;
    if (m == (m & mode))
    {
      console::print_d("%s%s", first ? "" : "|", da_profiler::profiler_mode_index_name(pi));
      first = false;
    }
  }
}
static void printf_all_modes(const char *c)
{
  String usage_str;
  usage_str.printf(0, "usage %s, where x: ", c);
  for (int pi = 0, pe = da_profiler::profiler_modes_count(); pi < pe; ++pi)
    usage_str.aprintf(0, "%s%s", pi == 0 ? "" : ",", da_profiler::profiler_mode_index_name(pi));
  console::print(usage_str);
}

static bool profiler_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
#define SET_SPIKE(v)                                                                                 \
  CONSOLE_CHECK_NAME("profiler", #v, 1, 2)                                                           \
  {                                                                                                  \
    uint32_t spike_min, spike_mul_average, spike_add;                                                \
    da_profiler::get_spike_parameters(spike_min, spike_mul_average, spike_add);                      \
    if (argc > 1)                                                                                    \
    {                                                                                                \
      v = atoi(argv[1]);                                                                             \
      da_profiler::set_spike_parameters(spike_min, spike_mul_average, spike_add);                    \
    }                                                                                                \
    da_profiler::get_spike_parameters(spike_min, spike_mul_average, spike_add);                      \
    console::print_d("current spike params: spike_min %d (ms), spike_mul %d, spike_add %d(ms)", /**/ \
      spike_min, spike_mul_average, spike_add);                                                      \
  }
  SET_SPIKE(spike_min)
  SET_SPIKE(spike_add)
  SET_SPIKE(spike_mul_average)

#define SET_SAMPLING(v)                                                                                                     \
  CONSOLE_CHECK_NAME("profiler", #v, 1, 2)                                                                                  \
  {                                                                                                                         \
    uint32_t sampling_frequency, sampling_spike_frequency, sampling_thread_mul;                                             \
    da_profiler::get_sampling_parameters(sampling_frequency, sampling_spike_frequency, sampling_thread_mul);                \
    if (argc > 1)                                                                                                           \
    {                                                                                                                       \
      v = atoi(argv[1]);                                                                                                    \
      da_profiler::set_sampling_parameters(sampling_frequency, sampling_spike_frequency, sampling_thread_mul);              \
    }                                                                                                                       \
    da_profiler::get_sampling_parameters(sampling_frequency, sampling_spike_frequency, sampling_thread_mul);                \
    console::print_d("current sampling params: sampling_frequency %d, sampling_spike_frequency %d, sampling_thread_mul %d", \
      sampling_frequency, sampling_spike_frequency, sampling_thread_mul);                                                   \
  }
  SET_SAMPLING(sampling_frequency)
  SET_SAMPLING(sampling_spike_frequency)
  SET_SAMPLING(sampling_thread_mul)
  CONSOLE_CHECK_NAME("profiler", "dump", 1, 1) { da_profiler::request_dump(); }
  CONSOLE_CHECK_NAME("profiler", "stop", 1, 2)
  {
    if (uint32_t mode = (argc > 1) ? da_profiler::find_profiler_mode(argv[1]) : 0)
      da_profiler::remove_mode(mode);
    else
      printf_all_modes("profiler.stop");
    show_current_modes();
  }
  CONSOLE_CHECK_NAME("profiler", "profile", 1, 2)
  {
    if (uint32_t mode = (argc > 1) ? da_profiler::find_profiler_mode(argv[1]) : 0)
      da_profiler::add_mode(mode);
    else
      printf_all_modes("profiler.profile");
    show_current_modes();
  }
  CONSOLE_CHECK_NAME("profiler", "log_event", 2, 2)
  {
    if (da_profiler::start_log_dump_server(argv[1]))
      console::print_d("logging gpu event %s", argv[1]);
  }
  CONSOLE_CHECK_NAME("profiler", "continuous", 3, 3)
  {
    int frames = to_int(argv[1]);
    int mbs = to_int(argv[2]);
    da_profiler::set_continuous_limits(frames, mbs);
    da_profiler::add_mode(da_profiler::CONTINUOUS);
    console::print_d("profiler limits set to %d frames, %d MB", frames, mbs);
  }
  // compatibility with old one
  CONSOLE_CHECK_NAME("profiler", "gpu", 1, 2)
  {
    if ((argc > 1) ? to_bool(argv[1]) : !(da_profiler::get_current_mode() & da_profiler::GPU))
      da_profiler::add_mode(da_profiler::EVENTS | da_profiler::GPU);
    else
      da_profiler::remove_mode(da_profiler::GPU);
    console::print_d("switched %s", (da_profiler::get_current_mode() & da_profiler::GPU) ? "on" : "off");
  }
  CONSOLE_CHECK_NAME("profiler", "enabled", 1, 2)
  {
    if ((argc > 1) ? to_bool(argv[1]) : !(da_profiler::get_current_mode() & da_profiler::EVENTS))
      da_profiler::add_mode(da_profiler::EVENTS);
    else
      da_profiler::remove_mode(da_profiler::EVENTS);
    console::print_d("switched %s", (da_profiler::get_current_mode() & da_profiler::EVENTS) ? "on" : "off");
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(profiler_console_handler);
#else
REGISTER_CONSOLE_PULL_VAR_NAME(profiler_console_handler)
#endif
