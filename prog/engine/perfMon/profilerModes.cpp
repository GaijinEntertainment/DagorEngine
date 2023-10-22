#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_daProfilerSettings.h>
#include <util/dag_globDef.h>
#include <string.h>

namespace da_profiler
{

static const char *profiler_options[] = {"all", "cpu", "tags", "gpu", "sampling", "save_spikes", "platform", nullptr};
static uint32_t profiler_options_val[] = {
  da_profiler::PLATFORM_EVENTS | da_profiler::EVENTS | da_profiler::TAGS | da_profiler::GPU | da_profiler::SAMPLING |
    da_profiler::SAVE_SPIKES,   // all
  da_profiler::EVENTS,          // cpu
  da_profiler::TAGS,            // tags
  da_profiler::GPU,             // gpu
  da_profiler::SAMPLING,        // sampling
  da_profiler::SAVE_SPIKES,     // save_spikes
  da_profiler::PLATFORM_EVENTS, // platform
  0,
};

uint32_t find_profiler_mode(const char *m)
{
  for (int pi = 0; profiler_options[pi]; ++pi)
    if (strcmp(m, profiler_options[pi]) == 0)
      return profiler_options_val[pi];
  return 0;
}

const char *profiler_mode_name(uint32_t mode)
{
  for (int pi = 0; profiler_options_val[pi]; ++pi)
    if ((mode & profiler_options_val[pi]) == mode)
      return profiler_options[pi];
  return nullptr;
}

uint32_t profiler_modes_count() { return countof(profiler_options) - 1; }

const char *profiler_mode_index_name(uint32_t index)
{
  return profiler_options[index < profiler_modes_count() ? index : profiler_modes_count()];
}
uint32_t profiler_mode_index(uint32_t index)
{
  return profiler_options_val[index < profiler_modes_count() ? index : profiler_modes_count()];
}

} // namespace da_profiler