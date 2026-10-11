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
  // mic sensitivity: the level AGC2 starts from
  float recordVolume = 1.0f;
  float playbackVolume = 1.0f;
  int recordDeviceId = -1;
  bool micEnabled = false;
  bool soundEnabled = false;
  // the mic records and its processed voice plays back to this user at the playback volume, silent while soundEnabled
  // is off; the capture reaches the sinks only while recording and micEnabled are set too. Every communicator with it
  // set plays the shared capture, so one owner sets it
  bool voiceTest = false;
};

constexpr float mic_level_floor_db = -100.0f;

// the mic after the record volume, the denoiser and the gain control, before the speech gate, over its latest 10 ms
// frame, in dBFS. gateOpen: the gate sends that frame now, not later with its preroll; true for every measured frame
// where no RNNoise speech gate runs
struct MicLevel
{
  float rmsDb = mic_level_floor_db;
  float peakDb = mic_level_floor_db;
  bool gateOpen = false;
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
  // true for a source that makes audio when asked: playback reads it a little ahead, one frame per process()
  // call, so it may repeat its frame until its own next update. A buffered source returns false and
  // reports its queue through hasAudioFrame
  virtual bool makesAudioOnDemand() const { return true; }
  // for a buffered source: true when receiveAudioFrame has audio now
  virtual bool hasAudioFrame() const { return false; }
  virtual bool isPositional() const { return false; }
  virtual float getMinDistance() const { return 0.0f; }
  virtual float getMaxDistance() const { return 0.0f; }
  virtual Point3 getSpeakerPos() const { return {}; }
  virtual float getVolume() const { return 1.0f; }
  virtual void resetQueue() {}
  // the playback ran dry for less than a frame inside a spurt this many times since the previous call; a longer dry
  // run starts the next spurt. Called on the voice worker thread
  virtual void onPlaybackUnderruns(int /*count*/) {}
  // the audio blocks the source dropped from its full receive queue since it was made, -1 when it does not count them.
  // Called on the voice worker thread
  virtual int getBlocksDroppedTotal() const { return -1; }
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

  // fails when the shared mic capture cannot start
  virtual bool init() = 0;
  // the record device and record volume are shared by all communicators: the last update wins
  virtual void updateSoundSettings(SoundSettings const &settings) = 0;
  // the mic records while any communicator has recording enabled and micEnabled set, or voiceTest set, or the console
  // switch voice.test is on. The switch plays back through the first communicator that sees it on, while every
  // communicator with voiceTest set plays back
  virtual void enableRecording() = 0;

  virtual void disableRecording() = 0;
  virtual void process() = 0;

  virtual unsigned getSampleRate() const = 0;
  virtual unsigned getBytesPerSample() const = 0;

  // the sink sample rate must match the channel; each sink gets frames of its own size
  virtual void addVoiceSink(IVoiceSink *sink) = 0;
  virtual void removeVoiceSink(IVoiceSink *sink) = 0;

  // Raw sinks receive unprocessed (pre-denoise) audio when raw output is enabled, only while
  // processed audio flows, and not sample-aligned with the processed stream.
  virtual void addRawVoiceSink(IVoiceSink *sink) = 0;
  virtual void removeRawVoiceSink(IVoiceSink *sink) = 0;

  // Toggle raw audio resampling/streaming. Thread-safe (can be called from worker threads).
  // Only registered raw sinks receive the raw audio.
  virtual void setRawOutputEnabled(bool enabled) = 0;

  virtual bool addVoiceSource(IVoiceSource *source) = 0;
  virtual void removeVoiceSource(IVoiceSource *source) = 0;
  virtual void updateSourceVolume(IVoiceSource *source) = 0;

  // the mic capture is shared by all communicators, and so is its level; silence while the mic does not record
  virtual MicLevel getMicLevel() const = 0;

  // hands the communicator to that thread; until then only the creating thread may call it, except the three getters and
  // setRawOutputEnabled. The communicator keeps the address, so the identity must outlive it
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

// the console variable voice.stats_enabled, off by default: the [VC] stats lines and the measurements only they need
bool are_stats_enabled();

// the most planned loss, for opusPacketLossPercent and for the loss that listeners report
static constexpr int MAX_OPUS_PACKET_LOSS_PERCENT = 50;
// the voice{tuning{}} keys opusBitrateBps, opusComplexity, opusInbandFec, opusPacketLossPercent and opusWidebandLossPercent,
// the numeric ones clamped, with these defaults. Opus writes the FEC only at a planned loss above 0 and above about 18 kbps
// at 10 percent; the planned loss also makes each frame lean less on the one before. With the FEC on, from
// opusWidebandLossPercent of planned loss on the encoder stays wideband (up to 8 kHz), so the FEC fits into the bitrate
struct OpusTuning
{
  int bitrateBps = 24000;
#if _TARGET_PC
  int complexity = 8;
#else
  // constraint: consoles and mobile keep the old complexity until a CPU check there
  int complexity = 4;
#endif
  bool inbandFec = true;
  int packetLossPercent = 10;
  // a value above MAX_OPUS_PACKET_LOSS_PERCENT never caps
  int widebandLossPercent = 2;
};
// any thread; the first call loads voice{tuning{}} of the game settings. generation gets the tuning generation of the values
OpusTuning opus_tuning(int *generation = nullptr);
// the voice{tuning{}} keys playoutMinJitterMs, playoutMaxJitterMs, playoutJitterMarginMs and playoutInitialJitterMs,
// clamped, with the defaults below for a missing key. A playout buffer takes a load at its next burst start, and keeps
// the jitter estimate it learned unless the load changes playoutInitialJitterMs
struct PlayoutTuning
{
  int minJitterMs = 20;
  int maxJitterMs = 200;
  int jitterMarginMs = 10;
  int initialJitterMs = 30;
};
// any thread; the first call loads voice{tuning{}} of the game settings. generation gets the tuning generation of the values
PlayoutTuning playout_tuning(int *generation = nullptr);
// any thread; 0 until the first tuning load, then it changes with each load, so a stage re-reads its values when it differs
int tuning_generation();

} // namespace voicechat
