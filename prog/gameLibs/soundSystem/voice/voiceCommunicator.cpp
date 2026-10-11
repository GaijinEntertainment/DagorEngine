// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>
#include <math/dag_mathBase.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_critSec.h>
#include <perfMon/dag_perfTimer2.h>
#include <util/dag_stdint.h>
#include <util/dag_finally.h>
#include <generic/dag_span.h>
#include <util/dag_string.h>

#include <EASTL/algorithm.h>
#include <EASTL/vector.h>
#include <generic/dag_relocatableFixedVector.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <fmod_errors.h>
#include <fmod.hpp>
#include <math.h>

#include <soundSystem/fmodApi.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/voiceCommunicator.h>
#include <timeSource/timeSource.h>
#include <rnnoise.h>

#include "helpers.h"
#include "voiceCapture.h"
#include "voiceFramer.h"
#include "voiceLoopback.h"
#include "voiceMetrics.h"
#include "voiceSourceControl.h"
#include "voiceSwitchVar.h"

static VoiceSwitchVar stats_enabled("voice.stats_enabled", false);
static VoiceSwitchVar test_enabled("voice.test", false, "plays the processed mic back, as SoundSettings::voiceTest does");
// constraint: the console test plays through one communicator only; for voiceTest of the settings the game makes that choice
static voicechat::VoiceCommunicator *volatile console_test_owner = nullptr;

namespace voicechat
{

class VoiceCommunicatorImpl final : public VoiceCommunicator
{
public:
  VoiceCommunicatorImpl(IVoiceSink *sink, SoundSettings const &settings);
  // a communicator that sent nothing also reports once, so the log tells an idle sink from one never attached
  ~VoiceCommunicatorImpl() override
  {
    interlocked_compare_exchange_ptr(console_test_owner, (VoiceCommunicator *)nullptr, (VoiceCommunicator *)this);
    if (syncStatsSwitch())
      reportSinkStats("communicator removed", true);
  }

  bool init() override;

  void updateSoundSettings(SoundSettings const &settings) override;

  void enableRecording() override;
  void disableRecording() override;

  void process() override;

  unsigned getSampleRate() const override { return chanSampleRate; }  // should be thread safe
  unsigned getBytesPerSample() const override { return sample_size; } // should be thread safe

  void addVoiceSink(IVoiceSink *sink) override;
  void removeVoiceSink(IVoiceSink *sink) override;

  void addRawVoiceSink(IVoiceSink *sink) override;
  void removeRawVoiceSink(IVoiceSink *sink) override;
  void setRawOutputEnabled(bool enabled) override;

  bool addVoiceSource(IVoiceSource *source) override;
  void removeVoiceSource(IVoiceSource *source) override;
  void updateSourceVolume(IVoiceSource *source) override;

  MicLevel getMicLevel() const override { return capture.micLevel(); }

  void registerThreadIdentity(const thread::Identity &id) override { threadIdentity = &id; };

private:
  // the creating thread sets the communicator up alone, then hands it to the registered thread
  bool isOwnerThread() const
  {
    return threadIdentity ? thread::Identity::check(threadIdentity) : get_current_thread_id() == creatorThreadId;
  }
  void updatePlayback(FMOD::System *fmod_sys);
  void publishPlaybackMetrics(int session);
  void updateRecord();
  bool isSending() const { return isRecording && soundSettings.micEnabled; }
  bool isVoiceTest() const { return soundSettings.voiceTest || consoleVoiceTest; }
  void updateMicWanted() { capture.setMicWanted(isSending() || isVoiceTest()); }
  void updateVoiceTest();
  bool syncConsoleTest();
  void sendToRawSinks(const int16_t *buf, unsigned int buf_size_samples, int64_t timestampUsec);
  // reads voice.stats_enabled for a report; the switch to on drops the frames counted while off
  bool syncStatsSwitch();
  void reportSinkStats(const char *reason, bool even_if_idle);

private:
  eastl::vector<VoiceFramer> framers;
  dag::RelocatableFixedVector<IVoiceSink *, 2, true> rawSinks;
  const thread::Identity *threadIdentity = nullptr;
  const int64_t creatorThreadId = get_current_thread_id();

  // constraint: declared before sourceControlMap, whose control of it must end first
  eastl::unique_ptr<VoiceLoopback> loopback;
  using VoiceSourceControlPtr = eastl::unique_ptr<VoiceSourceControl>;
  ska::flat_hash_map<IVoiceSource *, VoiceSourceControlPtr> sourceControlMap;
  eastl::vector<VoiceSourceControl::Metrics> playbackMetrics;

  const unsigned int chanSampleRate = 0;

  SoundSettings soundSettings;
  bool isRecording = false;
  bool isPortAttached = false;
  bool consoleVoiceTest = false;

  eastl::vector<int16_t> pcmBuf;
  eastl::vector<int16_t> rawBuf;
  VoiceCaptureClient capture;
  DagorRefTimeSource timeSource;
  int64_t sinkStatsStartUsec = 0;
  bool statsEnabled = false;
};

static constexpr int64_t sink_stats_period_usec = 300'000'000;
static constexpr int loopback_frame_ms = 10;
static constexpr int loopback_max_queued_ms = 500;

bool VoiceCommunicatorImpl::init()
{
  G_ASSERT(isOwnerThread());

  if (!sndsys::is_inited())
    return false;
  G_ASSERT_RETURN(!framers.empty(), false);
  if (!capture.init(soundSettings))
    return false;
  updateMicWanted();
  updateVoiceTest();
  return true;
}

bool VoiceCommunicatorImpl::syncConsoleTest()
{
  VoiceCommunicator *self = this;
  if (!test_enabled.load())
  {
    interlocked_compare_exchange_ptr(console_test_owner, (VoiceCommunicator *)nullptr, self);
    return false;
  }
  VoiceCommunicator *owner = interlocked_compare_exchange_ptr(console_test_owner, self, (VoiceCommunicator *)nullptr);
  return !owner || owner == self;
}

void VoiceCommunicatorImpl::updateVoiceTest()
{
  if (isVoiceTest() == bool(loopback))
    return;
  if (loopback)
  {
    removeVoiceSource(loopback.get());
    loopback.reset();
    return;
  }
  loopback = eastl::make_unique<VoiceLoopback>(chanSampleRate, ms_to_samples(loopback_frame_ms, chanSampleRate),
    ms_to_samples(loopback_max_queued_ms, chanSampleRate));
  if (addVoiceSource(loopback.get()))
    return;
  logwarn("[VC] the voice test cannot play the mic back");
  // a later settings update tries again
  removeVoiceSource(loopback.get());
  loopback.reset();
}

VoiceCommunicatorImpl::VoiceCommunicatorImpl(IVoiceSink *sink, SoundSettings const &ss) :
  soundSettings(ss), chanSampleRate(sink->getSampleRate()), capture(chanSampleRate)
{
  framers.emplace_back(*sink);
  debug("[VC] Voice channel sample rate %uhz, first sink frame of %u samples", chanSampleRate, sink->getFrameSamples());
}

void VoiceCommunicatorImpl::addVoiceSink(IVoiceSink *sink)
{
  G_ASSERT(isOwnerThread());
  G_ASSERT_RETURN(sink, );
  G_ASSERT_RETURN(sink->getSampleRate() == chanSampleRate, );
  framers.emplace_back(*sink);
}

void VoiceCommunicatorImpl::removeVoiceSink(IVoiceSink *sink)
{
  G_ASSERT(isOwnerThread());
  auto it = eastl::find_if(framers.begin(), framers.end(), [sink](const VoiceFramer &framer) { return framer.getSink() == sink; });
  if (it == framers.end())
    return;
  if (syncStatsSwitch())
    reportSinkStats(String(0, "sink %p removed", sink), true);
  framers.erase(it);
}

void VoiceCommunicatorImpl::sendToRawSinks(const int16_t *buf, unsigned int buf_size_samples, int64_t timestampUsec)
{
  for (IVoiceSink *sink : rawSinks)
    sink->sendAudioFrame(buf, buf_size_samples, timestampUsec);
}

void VoiceCommunicatorImpl::addRawVoiceSink(IVoiceSink *sink)
{
  G_ASSERT(isOwnerThread());
  G_ASSERT_RETURN(sink, );
  G_ASSERT_RETURN(sink->getSampleRate() == chanSampleRate, );
  rawSinks.push_back(sink);
}

void VoiceCommunicatorImpl::removeRawVoiceSink(IVoiceSink *sink)
{
  G_ASSERT(isOwnerThread());
  auto it = eastl::find(rawSinks.begin(), rawSinks.end(), sink);
  if (it != rawSinks.end())
    rawSinks.erase(it);
}

void VoiceCommunicatorImpl::setRawOutputEnabled(bool enabled) { capture.setRawWanted(enabled); }

bool VoiceCommunicatorImpl::addVoiceSource(IVoiceSource *source)
{
  G_ASSERT(isOwnerThread());

  G_ASSERT_RETURN(source, false);
  auto emplaceResult = sourceControlMap.emplace(source, nullptr);
  G_ASSERT_RETURN(emplaceResult.second, false);

  int mixerRate = 0;
  if (sndsys::is_inited())
  {
    FMOD_RESULT fmodRes = sndsys::fmodapi::get_system()->getSoftwareFormat(&mixerRate, nullptr, nullptr);
    CHECK_FMOD_RETURN_VAL(fmodRes, "getSoftwareFormat", false);
  }
  VoiceSourceControlPtr &control = emplaceResult.first->second;
  control = eastl::make_unique<VoiceSourceControl>(*source, mixerRate);

  if (!control->init(soundSettings.soundEnabled ? soundSettings.playbackVolume : 0.0f))
  {
    control.reset();
    logwarn("[VC] VoiceSourceControl::init() failed");
    return false;
  }

  return true;
}

void VoiceCommunicatorImpl::removeVoiceSource(IVoiceSource *source)
{
  G_ASSERT(isOwnerThread());

  auto it = sourceControlMap.find(source);
  if (it == sourceControlMap.end())
  {
    logwarn("[VC] attempt to remove an unknown voice source");
    return;
  }
  if (it->second)
    it->second->stop();
  sourceControlMap.erase(it);
}

void VoiceCommunicatorImpl::updateSourceVolume(IVoiceSource *source)
{
  G_ASSERT(isOwnerThread());

  auto it = sourceControlMap.find(source);
  if (it == sourceControlMap.end())
  {
    logwarn("[VC] attempt to update volume of an unknown voice source");
    return;
  }
  it->second->updateSourceVolume();
}

void VoiceCommunicatorImpl::updatePlayback(FMOD::System *fmod_sys)
{
  for (auto &[source, sourceControl] : sourceControlMap)
  {
    if (sourceControl)
      sourceControl->updatePlayback(fmod_sys);
  }
}

void VoiceCommunicatorImpl::publishPlaybackMetrics(int session)
{
  playbackMetrics.clear();
  for (auto &[source, sourceControl] : sourceControlMap)
    if (sourceControl)
      playbackMetrics.push_back(sourceControl->metrics(session));
  metrics::publish_playback(this, session, make_span_const(playbackMetrics));
}

void VoiceCommunicatorImpl::updateRecord()
{
  const int64_t curTime = timeSource.getTimeUsec();
  capture.read(pcmBuf, rawBuf);
  dag::ConstSpan<int16_t> pcmStream = make_span_const(pcmBuf);
  dag::ConstSpan<int16_t> rawStream = make_span_const(rawBuf);

  if (loopback)
    loopback->push(pcmStream);
  // the voice test alone records the mic; the sinks see such a capture as no capture
  if (!isSending())
    pcmStream = rawStream = {};

  if (!pcmStream.empty() && !rawStream.empty())
    sendToRawSinks(rawStream.data(), rawStream.size(), framers.front().streamTimeUsec(curTime));

  for (VoiceFramer &framer : framers)
    framer.push(pcmStream, curTime);
}

bool VoiceCommunicatorImpl::syncStatsSwitch()
{
  const bool statsOn = are_stats_enabled();
  if (statsOn && !statsEnabled)
  {
    for (VoiceFramer &framer : framers)
      framer.takeFramesSent();
    sinkStatsStartUsec = timeSource.getTimeUsec();
  }
  statsEnabled = statsOn;
  return statsOn;
}

void VoiceCommunicatorImpl::reportSinkStats(const char *reason, bool even_if_idle)
{
  const int64_t now = timeSource.getTimeUsec();
  const int seconds = (now - sinkStatsStartUsec) / 1000000;
  sinkStatsStartUsec = now;
  String sinks;
  int total = 0;
  // a sink is named by its address, since a removed sink shifts the positions of the others
  for (VoiceFramer &framer : framers)
  {
    const int frames = framer.takeFramesSent();
    total += frames;
    sinks.aprintf(0, "%ssink %p %d", sinks.empty() ? "" : ", ", framer.getSink(), frames);
  }
  if (even_if_idle || total > 0)
    debug("[VC] sink stats, %s, %d s: frames sent to %s", reason, seconds, sinks.str());
}

void VoiceCommunicatorImpl::process()
{
  G_ASSERT(isOwnerThread());

  if (!sndsys::is_inited())
    return;

  if (syncConsoleTest() != consoleVoiceTest)
  {
    consoleVoiceTest = !consoleVoiceTest;
    updateMicWanted();
    updateVoiceTest();
  }
  // before the push, so the first line after the switch keeps the frames of this call
  const bool statsOn = syncStatsSwitch();
  if (!framers.empty())
    updateRecord();

  updatePlayback(sndsys::fmodapi::get_system());
  if (const int metricsSession = metrics::active_session())
    publishPlaybackMetrics(metricsSession);

  if (statsOn && timeSource.getTimeUsec() - sinkStatsStartUsec >= sink_stats_period_usec)
    reportSinkStats("periodic", false);
}

void VoiceCommunicatorImpl::enableRecording()
{
  G_ASSERT(isOwnerThread());

  isRecording = true;
  updateMicWanted();
}

void VoiceCommunicatorImpl::disableRecording()
{
  G_ASSERT(isOwnerThread());

  isRecording = false;
  updateMicWanted();
}

void VoiceCommunicatorImpl::updateSoundSettings(SoundSettings const &settings)
{
  G_ASSERT(isOwnerThread());

  if (!sndsys::is_inited())
    return;

  if ((settings.playbackVolume != soundSettings.playbackVolume) || (settings.soundEnabled != soundSettings.soundEnabled))
  {
    float newVolume = settings.soundEnabled ? settings.playbackVolume : 0.0f;
    for (auto &[_, sourceControl] : sourceControlMap)
      if (sourceControl)
        sourceControl->setExternalVoiceVolume(newVolume);
  }

  soundSettings.micEnabled = settings.micEnabled;
  soundSettings.soundEnabled = settings.soundEnabled;
  soundSettings.playbackVolume = settings.playbackVolume;
  soundSettings.recordVolume = settings.recordVolume;
  soundSettings.recordDeviceId = settings.recordDeviceId;
  soundSettings.voiceTest = settings.voiceTest;
  capture.updateSettings(soundSettings);
  updateMicWanted();
  updateVoiceTest();
}

void VoiceCommunicatorAtomicGuard::init(eastl::unique_ptr<VoiceCommunicator> &&value)
{
  VoiceCommunicator *current = access.load();

  if (!current && value)
  {
    if (access.compare_exchange_strong(current, value.get()))
      ownership = eastl::move(value);
  }
}

void VoiceCommunicatorAtomicGuard::clear()
{
  access.store(nullptr);
  ownership.reset();
}

eastl::unique_ptr<VoiceCommunicator> create_voice_communicator(IVoiceSink *sink, SoundSettings const &settings)
{
  return eastl::make_unique<VoiceCommunicatorImpl>(sink, settings);
}

bool are_stats_enabled() { return stats_enabled.load(); }

} // namespace voicechat
