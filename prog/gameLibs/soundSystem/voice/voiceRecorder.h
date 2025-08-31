// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "speex_headers.h"
#include "voiceFeedback.h"
#include <EASTL/unique_ptr.h>
#include "helpers.h"

struct DenoiseState;

namespace voicechat
{

struct SoundSettings;

class VoiceRecorder
{
public:
  VoiceRecorder(int output_samplerate);
  ~VoiceRecorder();

  bool init(FMOD::System *fmod_sys, FMOD::ChannelGroup *channel_group);
  dag::ConstSpan<int16_t> readMicData(FMOD::System *fmod_sys, const SoundSettings &sound_settings);

  bool startRecord(FMOD::System *fmod_sys, const SoundSettings &sound_settings);
  void stopRecord(FMOD::System *fmod_sys, const SoundSettings &sound_settings);

private:
  bool createRecordSound(FMOD::System *fmod_sys);
  dag::Span<int16_t> postprocessFrame(dag::Span<int16_t> pcm_frame, const SoundSettings &sound_settings);
  dag::Span<int16_t> readFrames(FMOD::System *fmod_sys, const SoundSettings &sound_settings);

private:
  fmod_ptr<FMOD::Sound> recordSound;
  unsigned recordSoundSamples = 0;
  FMOD_CREATESOUNDEXINFO recordSoundInfo;

  eastl::unique_ptr<VoiceFeedback> feedback;
  SpeexEchoState *speexEchoSt = nullptr;
  SpeexPreprocessState *speexPPSt = nullptr;
  SpeexResamplerState *outputResampler = nullptr;

  DenoiseState *denoiseSt = nullptr;

  bool isRecording = false;
  int currentRecordDevId = -1;
  unsigned int recordPos = 0;

  eastl::vector<int16_t> PCMBuf1;
  eastl::vector<int16_t> PCMBuf2;
  eastl::vector<int16_t> outputBuf;
  eastl::vector<float> denoiseBuf;

  int outputSamplerate = 0;
  int silenceStartMs = 0;
};

} // namespace voicechat
