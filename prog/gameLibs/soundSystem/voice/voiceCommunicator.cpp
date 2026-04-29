// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>
#include <math/dag_mathBase.h>
#include <osApiWrappers/dag_critSec.h>
#include <perfMon/dag_perfTimer2.h>
#include <util/dag_stdint.h>
#include <util/dag_finally.h>
#include <generic/dag_span.h>

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
#include "voiceRecorder.h"
#include "voiceSourceControl.h"

#define VOICE_TRACE_ENABLED 0

#if VOICE_TRACE_ENABLED
#define TRACE_FRAME(fmt, ...) debug("[VC] " fmt, __VA_ARGS__)
#else
#define TRACE_FRAME(...) \
  {}
#endif


namespace voicechat
{

class VoiceCommunicatorImpl final : public VoiceCommunicator
{
public:
  VoiceCommunicatorImpl(IVoiceSink *sink, SoundSettings const &settings);
  ~VoiceCommunicatorImpl() = default;

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

  void registerThreadIdentity(const thread::Identity &id) override { threadIdentity = &id; };

private:
  void updatePlayback(FMOD::System *fmod_sys);
  void updateRecord(FMOD::System *fmod_sys);
  void sendToSinks(const int16_t *buf, unsigned int buf_size_samples, int64_t timestampUsec);
  void sendToRawSinks(const int16_t *buf, unsigned int buf_size_samples, int64_t timestampUsec);

  bool startRecord();
  void stopRecord();

  int64_t samplesToMcs(int num_samples);

private:
  FMOD::ChannelGroup *masterChannelGroup = nullptr;
  dag::RelocatableFixedVector<IVoiceSink *, 2, true> sinks;
  dag::RelocatableFixedVector<IVoiceSink *, 2, true> rawSinks;
  const thread::Identity *threadIdentity = nullptr;

  using VoiceSourceControlPtr = eastl::unique_ptr<VoiceSourceControl>;
  ska::flat_hash_map<IVoiceSource *, VoiceSourceControlPtr> sourceControlMap;

  const unsigned int chanSampleRate = 0;
  unsigned int chanFrameSamples = 0;
  unsigned int chanFrameBytes = 0;
  int64_t chanFrameMcs = 0;

  SoundSettings soundSettings;
  bool isRecording = false;
  bool isPortAttached = false;

  eastl::vector<int16_t> remainderBuf;
  dag::Span<int16_t> remainderSamplesToSend;
  int64_t remainderSendDeadline = 0;
  int64_t lastSentSamplesTime = 0;
  int64_t lastObtainedSamplesTime = 0;
  int64_t lastCheckForSamplesTime = 0;

  VoiceRecorder recorder;
  DagorRefTimeSource timeSource;
};

bool VoiceCommunicatorImpl::init()
{
  G_ASSERT(thread::Identity::check(threadIdentity));

  if (!sndsys::is_inited())
    return false;
  FMOD::System *fmodSys = sndsys::fmodapi::get_system();
  G_ASSERT_RETURN(!sinks.empty(), false);

  FMOD_RESULT fmodRes = fmodSys->getMasterChannelGroup(&masterChannelGroup);
  CHECK_FMOD_RETURN_VAL(fmodRes, "getMasterChannelGroup", false);

  return recorder.init(fmodSys, masterChannelGroup);
}

VoiceCommunicatorImpl::VoiceCommunicatorImpl(IVoiceSink *sink, SoundSettings const &ss) :
  soundSettings(ss), chanSampleRate(sink->getSampleRate()), chanFrameSamples(sink->getFrameSamples()), recorder(chanSampleRate)
{
  sinks.push_back(sink);
  chanFrameBytes = chanFrameSamples * sample_size;
  chanFrameMcs = samplesToMcs(chanFrameSamples);
  debug("[VC] Voice channel sample rate %uhz, frame of %u samples", chanSampleRate, chanFrameSamples);
  remainderBuf.resize(chanFrameSamples);
}

void VoiceCommunicatorImpl::addVoiceSink(IVoiceSink *sink)
{
  G_ASSERT(thread::Identity::check(threadIdentity));
  G_ASSERT_RETURN(sink, );
  G_ASSERT_RETURN(sink->getSampleRate() == chanSampleRate, );
  G_ASSERT_RETURN(sink->getFrameSamples() == chanFrameSamples, );
  sinks.push_back(sink);
}

void VoiceCommunicatorImpl::removeVoiceSink(IVoiceSink *sink)
{
  G_ASSERT(thread::Identity::check(threadIdentity));
  auto it = eastl::find(sinks.begin(), sinks.end(), sink);
  if (it != sinks.end())
    sinks.erase(it);
}

void VoiceCommunicatorImpl::sendToSinks(const int16_t *buf, unsigned int buf_size_samples, int64_t timestampUsec)
{
  for (IVoiceSink *sink : sinks)
    sink->sendAudioFrame(buf, buf_size_samples, timestampUsec);
}

void VoiceCommunicatorImpl::sendToRawSinks(const int16_t *buf, unsigned int buf_size_samples, int64_t timestampUsec)
{
  for (IVoiceSink *sink : rawSinks)
    sink->sendAudioFrame(buf, buf_size_samples, timestampUsec);
}

void VoiceCommunicatorImpl::addRawVoiceSink(IVoiceSink *sink)
{
  G_ASSERT(thread::Identity::check(threadIdentity));
  G_ASSERT_RETURN(sink, );
  G_ASSERT_RETURN(sink->getSampleRate() == chanSampleRate, );
  G_ASSERT_RETURN(sink->getFrameSamples() == chanFrameSamples, );
  rawSinks.push_back(sink);
}

void VoiceCommunicatorImpl::removeRawVoiceSink(IVoiceSink *sink)
{
  G_ASSERT(thread::Identity::check(threadIdentity));
  auto it = eastl::find(rawSinks.begin(), rawSinks.end(), sink);
  if (it != rawSinks.end())
    rawSinks.erase(it);
}

void VoiceCommunicatorImpl::setRawOutputEnabled(bool enabled) { recorder.setRawOutputEnabled(enabled); }

bool VoiceCommunicatorImpl::addVoiceSource(IVoiceSource *source)
{
  G_ASSERT(thread::Identity::check(threadIdentity));

  G_ASSERT_RETURN(source, false);
  auto emplaceResult = sourceControlMap.emplace(source, nullptr);
  G_ASSERT_RETURN(emplaceResult.second, false);

  VoiceSourceControlPtr &control = emplaceResult.first->second;
  control = eastl::make_unique<VoiceSourceControl>();

  if (!control->init(*source, soundSettings.soundEnabled ? soundSettings.playbackVolume : 0.0f))
  {
    control.reset();
    logwarn("[VC] VoiceSourceControl::init() failed");
    return false;
  }

  return true;
}

void VoiceCommunicatorImpl::removeVoiceSource(IVoiceSource *source)
{
  G_ASSERT(thread::Identity::check(threadIdentity));

  auto it = sourceControlMap.find(source);
  if (it == sourceControlMap.end())
  {
    logwarn("[VC] attempt to remove an unknown voice source");
    return;
  }
  sourceControlMap.erase(it);
}

void VoiceCommunicatorImpl::updateSourceVolume(IVoiceSource *source)
{
  G_ASSERT(thread::Identity::check(threadIdentity));

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

void VoiceCommunicatorImpl::updateRecord(FMOD::System *fmod_sys)
{
  int64_t curTime = timeSource.getTimeUsec();
  dag::ConstSpan<int16_t> rawStream;
  dag::ConstSpan<int16_t> pcmStream = recorder.readMicData(fmod_sys, soundSettings, rawSinks.empty() ? nullptr : &rawStream);

  // if deadline hit fill remainder with zeroes to full frame and send to voice
  if (pcmStream.empty())
  {
    if (!remainderSamplesToSend.empty())
    {
      if (remainderSendDeadline < curTime)
      {
        unsigned remSamplesCnt = remainderSamplesToSend.size();
        dag::Span<int16_t> zeroBuf = make_span(remainderSamplesToSend.data() + remSamplesCnt, chanFrameSamples - remSamplesCnt);
        mem_set_0(zeroBuf);
        TRACE_FRAME("fill remainder %d with %d zeroes and send", remSamplesCnt, zeroBuf.size());
        sendToSinks(remainderBuf.data(), remainderBuf.size(), lastSentSamplesTime);
        remainderSamplesToSend.reset();
        remainderSendDeadline = 0;
        lastObtainedSamplesTime = 0;
        lastSentSamplesTime = 0;
      }
    }
    else if (lastObtainedSamplesTime > 0 && lastCheckForSamplesTime > 0)
    {
      int64_t checkedIntervalMcs = lastCheckForSamplesTime - lastObtainedSamplesTime;
      if (checkedIntervalMcs > chanFrameMcs)
      {
        lastObtainedSamplesTime = 0;
        lastSentSamplesTime = 0;
      }
    }
  }

  lastCheckForSamplesTime = curTime;

  if (pcmStream.empty())
    return;

  if (!lastSentSamplesTime)
    lastSentSamplesTime = curTime;

  lastObtainedSamplesTime = curTime;

  // Send raw (pre-denoise) audio directly to raw sinks (no remainder framing needed;
  // the sink's ring buffer absorbs the bounded <=1 frame drift vs processed stream).
  if (!rawStream.empty())
    sendToRawSinks(rawStream.data(), rawStream.size(), lastSentSamplesTime);

  // part1. send remainder frame to voice
  if (!remainderSamplesToSend.empty())
  {
    unsigned remSamplesCnt = remainderSamplesToSend.size();
    unsigned remPart2SamplesCnt = eastl::min(chanFrameSamples - remSamplesCnt, pcmStream.size());
    dag::Span<int16_t> buf2 = make_span(remainderSamplesToSend.data() + remSamplesCnt, remPart2SamplesCnt);

    // we can eliminate this copy if sendAudioFrame accepts two buffers
    mem_copy_from(buf2, pcmStream.data());
    remainderSamplesToSend = make_span(remainderBuf.data(), remSamplesCnt + remPart2SamplesCnt);
    pcmStream = pcmStream.subspan(remPart2SamplesCnt);
    if (remainderSamplesToSend.size() == chanFrameSamples)
    {
      TRACE_FRAME("send remainder frame %d + %d at %ld", remSamplesCnt, remPart2SamplesCnt, lastSentSamplesTime / 1000);
      sendToSinks(remainderSamplesToSend.data(), remainderSamplesToSend.size(), lastSentSamplesTime);
      lastSentSamplesTime += chanFrameMcs;
      remainderSamplesToSend.reset();
    }
  }

  // part2. send whole frames to voice
  while (pcmStream.size() >= chanFrameSamples)
  {
    TRACE_FRAME("send full frame at %ld", lastSentSamplesTime / 1000);
    sendToSinks(pcmStream.data(), chanFrameSamples, lastSentSamplesTime);
    lastSentSamplesTime += chanFrameMcs;
    pcmStream = pcmStream.subspan(chanFrameSamples);
  }

  // part3. fill remainder buffer
  if (!pcmStream.empty())
  {
    remainderSamplesToSend = make_span(remainderBuf.data(), pcmStream.size());
    mem_copy_from(remainderSamplesToSend, pcmStream.data());
    remainderSendDeadline = lastSentSamplesTime + chanFrameMcs;
  }
}

void VoiceCommunicatorImpl::process()
{
  G_ASSERT(thread::Identity::check(threadIdentity));

  if (!sndsys::is_inited())
    return;
  FMOD::System *fmodSys = sndsys::fmodapi::get_system();
  if (isRecording && soundSettings.recordDeviceId >= 0)
  {
    bool isRealRecording = false;
    fmodSys->isRecording(soundSettings.recordDeviceId, &isRealRecording);
    if (!isRealRecording)
    {
      if (!startRecord())
      {
        // device not connected
        soundSettings.recordDeviceId = -1;
        return;
      }
    }
  }

  if (!sinks.empty())
    updateRecord(fmodSys);

  updatePlayback(fmodSys);
}

void VoiceCommunicatorImpl::enableRecording()
{
  G_ASSERT(thread::Identity::check(threadIdentity));

  if (!isRecording)
  {
    isRecording = true;
    startRecord();
  }
}

void VoiceCommunicatorImpl::disableRecording()
{
  G_ASSERT(thread::Identity::check(threadIdentity));

  if (isRecording)
  {
    isRecording = false;
    stopRecord();
  }
}

void VoiceCommunicatorImpl::updateSoundSettings(SoundSettings const &settings)
{
  G_ASSERT(thread::Identity::check(threadIdentity));

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

  if (soundSettings.recordDeviceId != settings.recordDeviceId)
  {
    stopRecord();
    soundSettings.recordDeviceId = settings.recordDeviceId;
    if (settings.recordDeviceId != -1)
      startRecord();
  }
}

bool VoiceCommunicatorImpl::startRecord()
{
  if (!sndsys::is_inited() || soundSettings.recordDeviceId < 0)
    return false;
  return recorder.startRecord(sndsys::fmodapi::get_system(), soundSettings);
}

void VoiceCommunicatorImpl::stopRecord()
{
  if (!sndsys::is_inited() || soundSettings.recordDeviceId < 0)
    return;
  recorder.stopRecord(sndsys::fmodapi::get_system(), soundSettings);
}

int64_t VoiceCommunicatorImpl::samplesToMcs(int num_samples) { return int64_t(num_samples) * 1000000 / chanSampleRate; }

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

} // namespace voicechat
