// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "edgeFader.h"
#include "helpers.h"
#include "speex_headers.h"
#include "spscRing.h"
#include "voiceTuning.h"
#include <fmod_common.h>


namespace voicechat
{

static const int sample_size = get_bytes_from_format(FMOD_SOUND_FORMAT_PCM16);


class IVoiceSource;

// Plays one voice source: the voice worker resamples its frames to the mixer rate into a ring, and a
// generator DSP plays the ring on the FMOD mixer thread, delayed by the edge fade. When the ring runs dry, the DSP
// fades the voice out and plays silence.
class VoiceSourceControl
{
public:
  VoiceSourceControl(IVoiceSource &source_, int mixer_rate);
  ~VoiceSourceControl();

  VoiceSourceControl(const VoiceSourceControl &) = delete;
  VoiceSourceControl &operator=(const VoiceSourceControl &) = delete;

  bool init(float volume);
  void updatePlayback(FMOD::System *fmod_sys);
  void setExternalVoiceVolume(float volume);
  void updateSourceVolume();
  // detaches from the mixer and gives the source the underruns it has not got yet; the source must still live
  void stop();

  // FMOD mixer thread
  void readMix(float *out, unsigned int length, int channels);

  struct Metrics
  {
    const IVoiceSource *source = nullptr;
    int ringQueuedMs = 0;
    int ringCapacityMs = 0;
    int underruns = 0;
    int blocksDropped = -1;
  };
  // voice worker: the counts start over at the first call of each overlay session; blocksDropped is -1 when the source
  // does not count them
  Metrics metrics(int session);

private:
  void applyTuning();
  void detachFromMixer();
  void update3dAttributes();
  void applyVolume();
  void pushFrames();

private:
  IVoiceSource &source;
  SpeexResamplerState *resampler = nullptr;
  FMOD::DSP *playbackDSP = nullptr;
  FMOD::ChannelGroup *voiceChannelGroup = nullptr;
  FMOD::Channel *playbackChannel = nullptr;
  eastl::vector<int16_t> frameBuf;
  eastl::vector<int16_t> resampledBuf;
  SpscRing<int16_t> ring;

  const unsigned chanSampleRate;
  const unsigned chanFrameSamples;
  const unsigned mixerRate;
  const unsigned mixerFrameSamples;
  float externalVolume = 0.0f;
  float localVolume = 1.0f;

  // the FMOD mixer thread owns these; silence starts at a frame, so the first spurt waits the margin too
  unsigned silentSamples = 0;
  unsigned marginLeft = 0;
  EdgeFader edgeFader;
  // the mixer adds, pushFrames on the voice worker takes
  volatile int underruns = 0;
  // the voice worker writes it, the mixer reads it at a spurt start
  volatile uint32_t marginSamples = 0;
  // the voice worker owns these
  PlaybackTuning tuning;
  // the part of the distance rolloff that the FMOD 3D level leaves out, applied through the channel volume
  float rolloffVolume = 1.0f;
  unsigned leadSamples = 0;
  int tuningGeneration = 0;
  int underrunsTaken = 0;
  int blocksDroppedStart = 0;
  int metricsSession = 0;
};

} // namespace voicechat
