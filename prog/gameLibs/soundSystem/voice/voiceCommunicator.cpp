// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>
#include <math/dag_mathBase.h>
#include <osApiWrappers/dag_critSec.h>
#include <perfMon/dag_perfTimer2.h>
#include <util/dag_stdint.h>
#include <util/dag_finally.h>
#include <generic/dag_span.h>

#include <EASTL/vector.h>
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

class VoiceCommunicatorImpl : public VoiceCommunicator
{
public:
  VoiceCommunicatorImpl(IVoiceSink *sink, SoundSettings const &settings);
  ~VoiceCommunicatorImpl() = default;

  bool init() override;

  void updateSoundSettings(SoundSettings const &settings) override;

  void enableRecording() override;
  void disableRecording() override;
  bool isRecordingEnabled() const override { return isRecording; }

  void process() override;

  unsigned getSampleRate() const override { return chanSampleRate; }
  unsigned getBytesPerSample() const override { return sample_size; }

  bool addVoiceSource(IVoiceSource *source) override;
  void removeVoiceSource(IVoiceSource *source) override;

private:
  void updatePlayback(FMOD::System *fmod_sys);
  void updateRecord(FMOD::System *fmod_sys);

  bool startRecord();
  void stopRecord();

  int64_t samplesToMcs(int num_samples);

private:
  FMOD::ChannelGroup *masterChannelGroup = nullptr;
  IVoiceSink *recordChannel = nullptr;

  ska::flat_hash_map<IVoiceSource *, VoiceSourceControl> sourceControlMap;

  unsigned int chanSampleRate = 0;
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

  VoiceRecorder recorder;
  DagorRefTimeSource timeSource;
};

bool VoiceCommunicatorImpl::init()
{
  if (!sndsys::is_inited())
    return false;
  FMOD::System *fmodSys = sndsys::fmodapi::get_system();
  G_ASSERT_RETURN(recordChannel, false);

  FMOD_RESULT fmodRes = fmodSys->getMasterChannelGroup(&masterChannelGroup);
  CHECK_FMOD_RETURN_VAL(fmodRes, "getMasterChannelGroup", false);

  return recorder.init(fmodSys, masterChannelGroup);
}

VoiceCommunicatorImpl::VoiceCommunicatorImpl(IVoiceSink *sink, SoundSettings const &ss) :
  recordChannel(sink),
  soundSettings(ss),
  chanSampleRate(recordChannel->getSampleRate()),
  chanFrameSamples(recordChannel->getFrameSamples()),
  recorder(chanSampleRate)
{
  chanFrameBytes = chanFrameSamples * sample_size;
  chanFrameMcs = samplesToMcs(chanFrameSamples);
  debug("[VC] Voice channel sample rate %uhz, frame of %u samples", chanSampleRate, chanFrameSamples);
  remainderBuf.resize(chanFrameSamples);
}

bool VoiceCommunicatorImpl::addVoiceSource(IVoiceSource *source)
{
  G_ASSERT_RETURN(source, false);
  auto emplaceResult = sourceControlMap.emplace(source, VoiceSourceControl{});
  G_ASSERT_RETURN(emplaceResult.second, false);
  VoiceSourceControl &control = emplaceResult.first->second;
  if (!control.init(*source, soundSettings.soundEnabled ? soundSettings.playbackVolume : 0.0f))
  {
    sourceControlMap.erase(emplaceResult.first);
    logwarn("[VC] VoiceSourceControl::init() failed");
    return false;
  }
  return true;
}

void VoiceCommunicatorImpl::removeVoiceSource(IVoiceSource *source)
{
  auto it = sourceControlMap.find(source);
  if (it == sourceControlMap.end())
  {
    logwarn("[VC] attempt to remove an unknown voice source");
    return;
  }
  sourceControlMap.erase(it);
}

void VoiceCommunicatorImpl::updatePlayback(FMOD::System *fmod_sys)
{
  for (auto &[_, sourceControl] : sourceControlMap)
    sourceControl.updatePlayback(fmod_sys);
}

void VoiceCommunicatorImpl::updateRecord(FMOD::System *fmod_sys)
{
  int64_t curTime = timeSource.getTimeUsec();
  dag::ConstSpan<int16_t> pcmStream = recorder.readMicData(fmod_sys, soundSettings);

  // if deadline hit fill remainder with zeroes to full frame and send to voice
  if (pcmStream.empty() && !remainderSamplesToSend.empty() && remainderSendDeadline < curTime)
  {
    unsigned remSamplesCnt = remainderSamplesToSend.size();
    dag::Span<int16_t> zeroBuf = make_span(remainderSamplesToSend.data() + remSamplesCnt, chanFrameSamples - remSamplesCnt);
    mem_set_0(zeroBuf);
    TRACE_FRAME("fill remainder %d with %d zeroes and send", remSamplesCnt, zeroBuf.size());
    recordChannel->sendAudioFrame(remainderBuf.data(), remainderBuf.size(), lastSentSamplesTime);
    remainderSamplesToSend.reset();
    remainderSendDeadline = 0;
    lastSentSamplesTime += chanFrameMcs;
  }

  if (pcmStream.empty())
    return;

  if (lastSentSamplesTime < curTime)
    lastSentSamplesTime = curTime;

  // part1. send remainder frame to voice
  if (!remainderSamplesToSend.empty())
  {
    unsigned remSamplesCnt = remainderSamplesToSend.size();
    unsigned remPart2SampelsCnt = eastl::min(chanFrameSamples - remSamplesCnt, pcmStream.size());
    dag::Span<int16_t> buf2 = make_span(remainderSamplesToSend.data() + remSamplesCnt, remPart2SampelsCnt);

    // we can eliminate this copy if sendAudioFrame accepts two buffers
    mem_copy_from(buf2, pcmStream.data());
    remainderSamplesToSend = make_span(remainderBuf.data(), remSamplesCnt + remPart2SampelsCnt);
    pcmStream = pcmStream.subspan(remPart2SampelsCnt);
    if (remainderSamplesToSend.size() == chanFrameSamples)
    {
      TRACE_FRAME("send remainder frame %d + %d at %ld", remSamplesCnt, remPart2SampelsCnt, lastSentSamplesTime / 1000);
      recordChannel->sendAudioFrame(remainderSamplesToSend.data(), remainderSamplesToSend.size(), lastSentSamplesTime);
      lastSentSamplesTime += chanFrameMcs;
      remainderSamplesToSend.reset();
    }
  }

  // part2. send whole frames to voice
  if (unsigned numFrames = pcmStream.size() / chanFrameSamples; numFrames > 0)
  {
    unsigned numSamples = numFrames * chanFrameSamples;
    recordChannel->sendAudioFrame(pcmStream.data(), numSamples, lastSentSamplesTime);
    TRACE_FRAME("send full frames %d at %ld", numSamples, lastSentSamplesTime / 1000);
    recordChannel->sendAudioFrame(remainderSamplesToSend.data(), remainderSamplesToSend.size(), lastSentSamplesTime);
    lastSentSamplesTime += numFrames * chanFrameMcs;
    pcmStream = pcmStream.subspan(numSamples);
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

  if (recordChannel)
    updateRecord(fmodSys);

  updatePlayback(fmodSys);
}

void VoiceCommunicatorImpl::enableRecording()
{
  if (!isRecording)
  {
    isRecording = true;
    startRecord();
  }
}

void VoiceCommunicatorImpl::disableRecording()
{
  if (isRecording)
  {
    isRecording = false;
    stopRecord();
  }
}

void VoiceCommunicatorImpl::updateSoundSettings(SoundSettings const &settings)
{
  if (!sndsys::is_inited())
    return;

  if ((settings.playbackVolume != soundSettings.playbackVolume) || (settings.soundEnabled != soundSettings.soundEnabled))
  {
    float newVolume = settings.soundEnabled ? settings.playbackVolume : 0.0f;
    for (auto &[_, sourceControl] : sourceControlMap)
      sourceControl.setGlobalVoiceVolume(newVolume);
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

eastl::unique_ptr<VoiceCommunicator> create_voice_communicator(IVoiceSink *sink, SoundSettings const &settings)
{
  return eastl::make_unique<VoiceCommunicatorImpl>(sink, settings);
}

} // namespace voicechat
