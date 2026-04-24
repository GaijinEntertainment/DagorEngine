//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/atomic.h>
#include <math/dag_Point3.h>
#include <util/dag_stdint.h>
#include <threadIdentity/threadIdentity.h>

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
  virtual bool isPositional() const { return false; }
  virtual float getMinDistance() const { return 0.0f; }
  virtual float getMaxDistance() const { return 0.0f; }
  virtual Point3 getSpeakerPos() const { return {}; }
  virtual float getVolume() const { return 1.0f; }
  virtual void resetQueue() {}
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
  virtual void process() = 0;

  virtual unsigned getSampleRate() const = 0;
  virtual unsigned getBytesPerSample() const = 0;

  virtual void addVoiceSink(IVoiceSink *sink) = 0;
  virtual void removeVoiceSink(IVoiceSink *sink) = 0;

  // Raw sinks receive unprocessed (pre-denoise) audio when raw output is enabled.
  virtual void addRawVoiceSink(IVoiceSink *sink) = 0;
  virtual void removeRawVoiceSink(IVoiceSink *sink) = 0;

  // Toggle raw audio resampling/streaming. Thread-safe (can be called from worker threads).
  // Only active when raw sinks are registered.
  virtual void setRawOutputEnabled(bool enabled) = 0;

  virtual bool addVoiceSource(IVoiceSource *source) = 0;
  virtual void removeVoiceSource(IVoiceSource *source) = 0;
  virtual void updateSourceVolume(IVoiceSource *source) = 0;

  virtual void registerThreadIdentity(const thread::Identity &id) = 0;
};

class VoiceCommunicatorAtomicGuard
{
public:
  void init(eastl::unique_ptr<VoiceCommunicator> &&value);
  void clear();

  const voicechat::VoiceCommunicator *load() const { return access.load(); }
  voicechat::VoiceCommunicator *load() { return access.load(); }

private:
  eastl::unique_ptr<VoiceCommunicator> ownership;
  eastl::atomic<voicechat::VoiceCommunicator *> access;
};

eastl::unique_ptr<VoiceCommunicator> create_voice_communicator(IVoiceSink *sink, SoundSettings const &settings);

} // namespace voicechat
