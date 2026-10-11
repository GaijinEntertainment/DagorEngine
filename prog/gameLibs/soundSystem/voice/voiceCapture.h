// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <soundSystem/voiceCommunicator.h>
#include <EASTL/vector.h>

namespace voicechat
{

class SharedVoiceCapture;

// One communicator's share of the single mic capture of the process.
// The capture records while any client wants the mic; each client gets the audio captured while it wants it,
// and a client that stops wanting the mic drops its unread audio.
// Each client gets the audio at its own sample rate.
class VoiceCaptureClient
{
public:
  explicit VoiceCaptureClient(unsigned sample_rate_) : sampleRate(sample_rate_) {}
  ~VoiceCaptureClient();

  VoiceCaptureClient(const VoiceCaptureClient &) = delete;
  VoiceCaptureClient &operator=(const VoiceCaptureClient &) = delete;

  bool init(const SoundSettings &settings);
  void setMicWanted(bool wanted);
  // thread-safe
  void setRawWanted(bool wanted);
  // record device and volume are shared: the last update of any client wins
  void updateSettings(const SoundSettings &settings);
  // moves the audio captured since the previous read, at most the newest 500 ms; raw stays empty when not wanted
  void read(eastl::vector<int16_t> &pcm, eastl::vector<int16_t> &raw);
  // thread-safe after init
  MicLevel micLevel() const;

private:
  friend class SharedVoiceCapture;

  SharedVoiceCapture *capture = nullptr;
  eastl::vector<int16_t> pcmQueue;
  eastl::vector<int16_t> rawQueue;
  const unsigned sampleRate;
  unsigned pcmSkip = 0;
  unsigned rawSkip = 0;
  volatile int rawWanted = 0;
  bool micWanted = false;
  bool micWantedLastStep = false;
  bool receiving = false;
  bool lagReported = false;
};

} // namespace voicechat
