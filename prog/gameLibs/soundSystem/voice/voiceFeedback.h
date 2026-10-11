// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <EASTL/vector.h>
#include <fmod.hpp>
#include "speex_headers.h"
#include "spscRing.h"

namespace voicechat
{

// The echo reference: what the master channel group plays, as 10 ms frames at the capture rate.
// processEchoSource runs on the FMOD mixer thread, the other members on the thread that owns the recorder.
class VoiceFeedback
{
public:
  // reference older than window_ms cannot line up with the mic capture and is dropped
  VoiceFeedback(unsigned int sample_rate, unsigned int frame_samples, unsigned int mixer_rate, int window_ms);
  ~VoiceFeedback();

  bool init(FMOD::System *fmod_sys, FMOD::ChannelGroup *channel_group);
  void shutdown();

  void processEchoSource(const float *inbuffer, unsigned int nsamples, int nchannels);

  bool popFrame(dag::Span<int16_t> out);

  // enable drops the reference left from before; disable logs how much reference was lost
  void enable();
  void disable();

private:
  void dropPending();
  void restart();
  void drainRing();

  SpeexResamplerState *resampler = nullptr;
  FMOD::DSP *captureDSP = nullptr;
  FMOD::ChannelGroup *channelGroup = nullptr;

  eastl::vector<float> mixerRateBuf;
  eastl::vector<float> resampledBuf;
  eastl::vector<int16_t> pendingPcm;
  SpscRing<float> ring;

  const int samplerate = 0;
  const unsigned int frameSize = 0;
  const int mixerRate = 0;
  const uint32_t windowSamples = 0;
  const uint32_t fullSamples = 0;
  volatile int enabled = 0;
  volatile uint32_t droppedSamples = 0;
  uint32_t discardedTotal = 0;
};
} // namespace voicechat
