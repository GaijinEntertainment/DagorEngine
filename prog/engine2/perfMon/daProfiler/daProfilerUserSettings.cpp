#include "daProfilerInternal.h"
#include <ioSys/dag_genIo.h>

// this is default receivers (from web/live to game)
namespace da_profiler
{

void UserSettings::setSpikes(uint32_t min_msec, uint32_t average_mul, uint32_t add_msec)
{
  interlocked_release_store(spikeMinMsec, min_msec);
  interlocked_release_store(spikeAvgMul, average_mul);
  interlocked_release_store(spikeAddMsec, add_msec);
  interlocked_increment(generation);
}
void UserSettings::setSpikeSave(uint32_t before, uint32_t after)
{
  interlocked_release_store(framesBeforeSpike, before);
  interlocked_release_store(framesAfterSpike, after);
  interlocked_increment(generation);
}

void UserSettings::setContinuousLimits(uint32_t frames, uint32_t size_mb)
{
  interlocked_release_store(continuousLimitFrames, frames);
  interlocked_release_store(continuousLimitSizeMb, size_mb);
  interlocked_increment(generation);
}

void UserSettings::setSampling(uint32_t sampling_rate_freq, uint32_t sampling_spike_rate_freq, uint32_t threads_sampling_rate_mul)
{
  interlocked_release_store(samplingFreq, sampling_rate_freq);
  interlocked_release_store(spikeSamplingFreq, sampling_spike_rate_freq);
  interlocked_release_store(threadsSamplingRate, threads_sampling_rate_mul);
  interlocked_increment(generation);
}

#define PERFORM_ALL                        \
  PERFORM(settings.mode);                  \
  PERFORM(settings.samplingFreq);          \
  PERFORM(settings.spikeSamplingFreq);     \
  PERFORM(settings.threadsSamplingRate);   \
  PERFORM(settings.spikeMinMsec);          \
  PERFORM(settings.spikeAvgMul);           \
  PERFORM(settings.spikeAddMsec);          \
  PERFORM(settings.continuousLimitFrames); \
  PERFORM(settings.continuousLimitSizeMb); \
  PERFORM(settings.framesBeforeSpike);     \
  PERFORM(settings.framesAfterSpike);      \
  PERFORM(settings.ringBufferSize);

bool read_settings(IGenLoad &load_cb, UserSettings &settings, uint32_t len)
{
  int lenLeft = len;
#define PERFORM(val)                                     \
  if (lenLeft < sizeof(val))                             \
    return false;                                        \
  if (load_cb.tryRead(&val, sizeof(val)) != sizeof(val)) \
    return false;                                        \
  lenLeft -= sizeof(val);
  PERFORM_ALL
#undef PERFORM
  char buf[64];
  for (; lenLeft > 0;)
  {
    const size_t sz = min(sizeof(buf), (size_t)len);
    load_cb.read(buf, sz);
    lenLeft -= int(sz);
  }
  return true;
}

uint32_t write_settings(IGenSave &cb, const UserSettings &settings)
{
  uint32_t len = 0;
#define PERFORM(val)           \
  cb.write(&val, sizeof(val)); \
  len += sizeof(val)
  PERFORM_ALL
#undef PERFORM
  return len;
}

void set_settings(const UserSettings &s)
{
  set_spike_parameters(s.spikeMinMsec, s.spikeAvgMul, s.spikeAddMsec);
  set_spike_save_parameters(s.framesBeforeSpike, s.framesAfterSpike);
  set_sampling_parameters(s.samplingFreq, s.spikeSamplingFreq, s.threadsSamplingRate);
  set_mode(s.mode);
  set_continuous_limits(s.continuousLimitFrames, s.continuousLimitSizeMb);
}

} // namespace da_profiler
