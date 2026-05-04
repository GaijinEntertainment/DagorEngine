// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "helpers.h"
#include <fmod_common.h>


namespace voicechat
{

static const FMOD_SOUND_FORMAT process_sample_format = FMOD_SOUND_FORMAT_PCM16;
static const int sample_size = get_bytes_from_format(process_sample_format);
static const int playback_frames_count = 4;


class IVoiceSource;

class VoiceSourceControl
{
public:
  bool init(IVoiceSource &source_, float volume);
  void updatePlayback(FMOD::System *fmod_sys);
  void setExternalVoiceVolume(float volume);
  void updateSourceVolume();

private:
  void update3dAttributes();

private:
  IVoiceSource *source = nullptr;
  fmod_ptr<FMOD::Sound> playbackSound;
  FMOD::ChannelGroup *voiceChannelGroup = nullptr;
  FMOD::Channel *playbackChannel = nullptr;
  float externalVolume = 0.0f;
  float localVolume = 1.0f;
  unsigned chanSampleRate = 0;
  unsigned chanFrameSamples = 0;
  unsigned chanFrameBytes = 0;
  int playbackLastFilledFrame = -1;
  bool isPositional = false;
};

} // namespace voicechat
