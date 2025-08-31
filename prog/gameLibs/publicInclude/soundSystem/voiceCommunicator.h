//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <util/dag_stdint.h>

namespace voicechat
{

struct SoundSettings
{
  float recordVolume = 1.0f;
  float playbackVolume = 1.0f;
  int recordDeviceId = -1;
  bool micEnabled = false;
  bool soundEnabled = false;
  bool voiceTest = false;
};

class IVoiceTransport
{
public:
  IVoiceTransport() = default;
  virtual ~IVoiceTransport() = default;

  IVoiceTransport(const IVoiceTransport &) = delete;
  IVoiceTransport &operator=(const IVoiceTransport &) = delete;
  IVoiceTransport(IVoiceTransport &&) = default;
  IVoiceTransport &operator=(IVoiceTransport &&) = default;

  virtual uint32_t getSampleRate() const = 0;
  virtual uint32_t getFrameSamples() const = 0;
};

class IVoiceSource : public IVoiceTransport
{
public:
  virtual void receiveAudioFrame(int16_t *buf, unsigned int buf_size_samples) = 0;
};

class IVoiceSink : public IVoiceTransport
{
public:
  virtual void sendAudioFrame(const int16_t *buf, unsigned int buf_size_samples, int64_t timestampUsec) = 0;
};

class VoiceCommunicator
{
public:
  virtual ~VoiceCommunicator() = default;

  virtual bool init() = 0;
  virtual void updateSoundSettings(SoundSettings const &settings) = 0;
  virtual void enableRecording() = 0;

  virtual void disableRecording() = 0;
  virtual bool isRecordingEnabled() const = 0;
  virtual void process() = 0;

  virtual unsigned getSampleRate() const = 0;
  virtual unsigned getBytesPerSample() const = 0;

  virtual bool addVoiceSource(IVoiceSource *source) = 0;
  virtual void removeVoiceSource(IVoiceSource *source) = 0;
};

eastl::unique_ptr<VoiceCommunicator> create_voice_communicator(IVoiceSink *sink, SoundSettings const &settings);

} // namespace voicechat
