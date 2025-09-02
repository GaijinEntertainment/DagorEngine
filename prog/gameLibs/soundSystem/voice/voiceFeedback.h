// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <EASTL/vector.h>
#include <fmod.hpp>
#include <osApiWrappers/dag_critSec.h>
#include <EASTL/array.h>
#include <EASTL/queue.h>
#include "speex_headers.h"

namespace voicechat
{

class VoiceFeedback
{
public:
  VoiceFeedback(unsigned int sample_rate, FMOD_SOUND_FORMAT sample_format, unsigned int frame_samples);
  ~VoiceFeedback();

  bool init(FMOD::System *fmod_sys, FMOD::ChannelGroup *channel_group);
  void shutdown();

  void processEchoSource(int samplerate_in, const float *inbuffer, unsigned int nsamples, int nchannels);

  dag::ConstSpan<int16_t> lockFrame();
  void unlockFrame();

  unsigned getMaxFramesCount() const { return framesCount; }

  void enable();
  void disable();

private:
  struct PCMFrame
  {
    dag::Span<int16_t> samples;
    int age = 0;
  };

  PCMFrame *nextFrameFromPool();

  int samplerate = 0;
  bool enabled = false;

  SpeexResamplerState *resampler = nullptr;
  FMOD::DSP *captureDSP = nullptr;
  FMOD::ChannelGroup *channelGroup = nullptr;

  eastl::vector<float> f32PCMBuffer;
  eastl::vector<int16_t> i16PCMProcess;
  eastl::vector<int16_t> i16PCMOutput;

  static constexpr unsigned int framesCount = 10;

  unsigned int frameSize = 0;
  unsigned int sampleSize = 0;


  int idCounter = 0;
  std::array<PCMFrame, framesCount> framesPool;
  eastl::queue<PCMFrame *> outFrames;
  WinCritSec mutex;
};
} // namespace voicechat
