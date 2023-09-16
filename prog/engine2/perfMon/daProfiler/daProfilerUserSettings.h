#include <stdint.h>
#include <osApiWrappers/dag_atomic.h>
#include "daProfilerDefines.h"
#undef DA_PROFILER_ENABLED
#define DA_PROFILER_ENABLED 1
#include <perfMon/dag_daProfiler.h>

class IGenLoad;
class IGenSave;

namespace da_profiler
{

#if DAPROFILER_DEBUGLEVEL > 1 || DAGOR_THREAD_SANITIZER || DAGOR_ADDRESS_SANITIZER
static constexpr uint32_t default_mode = 0; // off in all slow modes
#elif !_TARGET_STATIC_LIB
static constexpr uint32_t default_mode = 0;
#elif DAPROFILER_DEBUGLEVEL == 1 && _TARGET_PC_WIN
static constexpr uint32_t default_mode = EVENTS | SAMPLING | TAGS | SAVE_SPIKES;
#elif DAPROFILER_DEBUGLEVEL == 1
static constexpr uint32_t default_mode = SAMPLING | TAGS | SAVE_SPIKES; // globally is off on non-PC by default. Unlike PC everyone
                                                                        // else can not /should not save files, so there is no
                                                                        // profiling client by default
#else
static constexpr uint32_t default_mode = SAMPLING | TAGS | SAVE_SPIKES | GPU; // gpu is on, save_spikes is off, but global is off
#endif

struct UserSettings
{
  uint32_t mode = default_mode;
  uint32_t samplingFreq = 100;
  uint32_t spikeSamplingFreq = 1000;
  uint32_t threadsSamplingRate = 3;
  uint32_t spikeMinMsec = 100; // 100msec
  uint32_t spikeAvgMul = 3;
  uint32_t spikeAddMsec = 5; // add 1msec
  uint32_t continuousLimitFrames = 0;
  uint32_t continuousLimitSizeMb = 1024;
  uint32_t framesBeforeSpike = 3;
  uint32_t framesAfterSpike = 2;
  uint32_t ringBufferSize = 256;
  uint32_t generation = 0;

  uint32_t getMode() const { return interlocked_acquire_load(mode); }

  void setMode(uint32_t new_mode)
  {
    interlocked_release_store(mode, new_mode);
    interlocked_increment(generation);
  }
  void addMode(uint32_t mask)
  {
    interlocked_or(mode, mask);
    interlocked_increment(generation);
  }

  void removeMode(uint32_t mask)
  {
    interlocked_and(mode, ~mask);
    interlocked_increment(generation);
  }

  void setSpikes(uint32_t min_msec, uint32_t average_mul, uint32_t add_msec);
  void setSpikeSave(uint32_t before, uint32_t after);
  void setSampling(uint32_t sampling_rate_freq, uint32_t sampling_spike_rate_freq, uint32_t threads_sampling_rate_mul);
  void setContinuousLimits(uint32_t frames, uint32_t size_mb);
};

bool read_settings(IGenLoad &cb, UserSettings &settings, uint32_t len);
uint32_t write_settings(IGenSave &cb, const UserSettings &settings);
void set_settings(const UserSettings &s);
void send_settings(const UserSettings &s); // from game to tool

} // namespace da_profiler
