// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_daProfilerSettings.h>

#if TIME_PROFILER_ENABLED
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>

namespace da_profiler
{

static void set_profiling_profile_settings(const char *profilerSetting)
{
  static const char *profiler_legacy_options[] = {"all", "gpu", "cpu", "platform", "off", nullptr};
  static uint32_t profiler_legacy_options_val[] = {
    da_profiler::PLATFORM_EVENTS | da_profiler::EVENTS | da_profiler::UNIQUE_EVENTS | da_profiler::TAGS | da_profiler::GPU |
      da_profiler::SAMPLING | da_profiler::SAVE_SPIKES,
    da_profiler::EVENTS | da_profiler::UNIQUE_EVENTS | da_profiler::GPU | da_profiler::TAGS,
    da_profiler::EVENTS | da_profiler::TAGS,
    da_profiler::PLATFORM_EVENTS,
    0,
  };
  int pi = 0;
  for (; profiler_legacy_options[pi]; ++pi)
    if (strcmp(profilerSetting, profiler_legacy_options[pi]) == 0)
      break;
  if (profiler_legacy_options[pi])
    da_profiler::set_mode(profiler_legacy_options_val[pi]);
  else
    logwarn("'debug/profiler config is invalid (%s)! Using 'off' instead.", profilerSetting);
}

void set_profiling_settings(const DataBlock &settings)
{
  // we can just execute console commaned profiler.set, but meaning of values is now different
  if (settings.paramExists("profiler")) // old way
    set_profiling_profile_settings(settings.getStr("profiler", "off"));
  const DataBlock *profiler = settings.getBlockByName("profiler");
  if (!profiler)
    return;
  if (profiler->paramExists("profile"))
  {
    int paramId = -1;
    uint32_t mode = 0;
    while ((paramId = profiler->findParam("profile", paramId)) >= 0)
      mode |= find_profiler_mode(profiler->getStr(paramId));
    da_profiler::set_mode(mode);
  }
  if (const DataBlock *sampling = profiler->getBlockByName("sampling"))
  {
    uint32_t freq, spikeFreq, threadMul;
    da_profiler::get_sampling_parameters(freq, spikeFreq, threadMul);
    freq = sampling->getInt("frequency", freq);
    spikeFreq = sampling->getInt("spike_frequency", spikeFreq);
    threadMul = sampling->getInt("thread_mul", threadMul);
    da_profiler::set_sampling_parameters(freq, spikeFreq, threadMul);
  }
  if (const DataBlock *spikes = profiler->getBlockByName("spikes"))
  {
    uint32_t minMsec, averageMul, addMsec;
    da_profiler::get_spike_parameters(minMsec, averageMul, addMsec);
    minMsec = spikes->getInt("minMsec", minMsec);
    averageMul = spikes->getInt("averageMul", averageMul);
    addMsec = spikes->getInt("addMsec", addMsec);
    da_profiler::set_spike_parameters(minMsec, averageMul, addMsec);

    uint32_t framesBefore, framesAfter;
    da_profiler::get_spike_save_parameters(framesBefore, framesAfter);
    framesBefore = spikes->getInt("framesBefore", framesBefore);
    framesAfter = spikes->getInt("framesAfter", framesAfter);

    da_profiler::set_spike_save_parameters(framesBefore, framesAfter);
  }
}

} // namespace da_profiler
#else
namespace da_profiler
{
void set_profiling_settings(const DataBlock &) {}
} // namespace da_profiler
#endif // #if TIME_PROFILE_ENABLED
