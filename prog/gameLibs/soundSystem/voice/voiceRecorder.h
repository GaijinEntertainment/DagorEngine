// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "voiceCaptureDump.h"
#include "voiceFeedback.h"
#include "voiceProcessor.h"
#include <EASTL/unique_ptr.h>
#include "helpers.h"

namespace voicechat
{

struct SoundSettings;

class VoiceRecorder
{
public:
  // counts since the previous takeStats
  struct Stats
  {
    // reads that lagged past half the record ring; a lag past the whole ring cannot be seen
    int lags = 0;
    int maxLagMs = 0;
    ProcessingStats processing;

    void add(const Stats &other);
  };

  VoiceRecorder(int output_samplerate);
  ~VoiceRecorder();

  bool init(FMOD::System *fmod_sys, FMOD::ChannelGroup *channel_group);
  // Returns processed (denoised) audio, speech-gated: it may be empty or start with earlier frames.
  // want_raw also makes rawOutput give the raw (pre-denoise) audio. The span is valid until the next call.
  // Must only be called from the voice capture thread.
  dag::ConstSpan<int16_t> readMicData(FMOD::System *fmod_sys, const SoundSettings &sound_settings, bool want_raw);
  // the rates of VoiceProcessor::setOutputRates; readMicData returns the first, and a capture dump at a rate that
  // leaves the set ends
  void setOutputRates(dag::ConstSpan<int> rates);
  // the last readMicData result at any output rate, as VoiceProcessor::output and rawOutput give it
  dag::ConstSpan<int16_t> output(int rate) const { return processor.output(rate); }
  dag::ConstSpan<int16_t> rawOutput(int rate) const { return processor.rawOutput(rate); }

  // a started recording starts the counts from zero
  bool startRecord(FMOD::System *fmod_sys, const SoundSettings &sound_settings);
  Stats takeStats();
  void dropSpeechGatePreroll();
  void setStatsEnabled(bool enabled) { processor.setStatsEnabled(enabled); }
  // the voice capture thread, like readMicData
  MicLevel micLevel() const { return processor.micLevel(); }
  void stopRecord(FMOD::System *fmod_sys, const SoundSettings &sound_settings);

private:
  bool createRecordSound(FMOD::System *fmod_sys);
  // whole capture frames read from the record ring since the previous call
  dag::ConstSpan<int16_t> readFrames(FMOD::System *fmod_sys);

private:
  VoiceProcessor processor;
  fmod_ptr<FMOD::Sound> recordSound;
  FMOD_CREATESOUNDEXINFO recordSoundInfo;
  eastl::unique_ptr<VoiceFeedback> feedback;
  eastl::vector<int16_t> captureBuf;
#if DAGOR_DBGLEVEL > 0
  // hands the dump, if any, to a thread of its own that writes it, so the capture never waits for the files
  void finishDump();
  eastl::unique_ptr<CaptureDump> dump;
#endif
  Stats stats;

  unsigned recordSoundSamples = 0;
  int currentRecordDevId = -1;
  unsigned int recordPos = 0;
  bool isRecording = false;
};

} // namespace voicechat
