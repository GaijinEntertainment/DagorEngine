// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>
#include <math/dag_mathBase.h>
#include <osApiWrappers/dag_critSec.h>
#include <perfMon/dag_perfTimer2.h>
#include <util/dag_stdint.h>

#include <EASTL/vector.h>
#include <fmod_errors.h>
#include <fmod.hpp>
#include <math.h>

#include <soundSystem/fmodApi.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/voiceCommunicator.h>
#include <util/dag_convar.h>

#define OUTSIDE_SPEEX
#define RANDOM_PREFIX op4s_speex
#include <speex/speex_echo.h>
#include <speex/speex_resampler.h>

#define speex_preprocess_state_init    op4s_speex_preprocess_state_init
#define speex_preprocess_state_destroy op4s_speex_preprocess_state_destroy
#define speex_preprocess_ctl           op4s_speex_preprocess_ctl
#define speex_preprocess_run           op4s_speex_preprocess_run
#include <speex/speex_preprocess.h>

// #define DEBUG_SND_PROCESSING

#if DAGOR_DBGLEVEL > 0
static CONSOLE_INT_VAL("voice", mic_buf_duration_ms, 100, 10, 200);
#else
constexpr int mic_buf_duration_ms = 100;
#endif

#define CHECK_FMOD(res, message) \
  if (res != FMOD_OK)            \
    LOGWARN_CTX("[VC] %s %s: %s", __FUNCTION__, message, FMOD_ErrorString(res));

#define CHECK_FMOD_RETURN(res, message)                                           \
  if (res != FMOD_OK)                                                             \
  {                                                                               \
    LOGWARN_CTX("[VC] %s: %s: %s", __FUNCTION__, message, FMOD_ErrorString(res)); \
    return;                                                                       \
  }

#define CHECK_FMOD_RETURN_VAL(res, message, ret)                                  \
  if (res != FMOD_OK)                                                             \
  {                                                                               \
    LOGWARN_CTX("[VC] %s: %s: %s", __FUNCTION__, message, FMOD_ErrorString(res)); \
    return ret;                                                                   \
  }

namespace voicechat
{

struct ReleaseDeleter
{
  template <typename T>
  void operator()(T *ptr)
  {
    if (ptr)
      ptr->release();
  }
};

template <typename T>
using fmod_ptr = eastl::unique_ptr<T, ReleaseDeleter>;

static inline int get_bits_from_format(FMOD_SOUND_FORMAT format)
{
  switch (format)
  {
    case FMOD_SOUND_FORMAT_PCM8: return 8;
    case FMOD_SOUND_FORMAT_PCM16: return 16;
    case FMOD_SOUND_FORMAT_PCM24: return 24;
    case FMOD_SOUND_FORMAT_PCM32: return 32;
    case FMOD_SOUND_FORMAT_PCMFLOAT: return 32;
    default: break;
  }
  return 8;
}

static int get_bytes_from_format(FMOD_SOUND_FORMAT format) { return get_bits_from_format(format) / 8; }

static int reRate(int value, unsigned rate_in, unsigned rate_out) { return ceilf(value * float(rate_out) / float(rate_in)); }

static int samplesToMs(int samples, unsigned rate) { return ceilf(samples * 1000.0f / rate); }


static const FMOD_SOUND_FORMAT process_sample_format = FMOD_SOUND_FORMAT_PCM16;
static const int sample_size = get_bytes_from_format(process_sample_format);

// record stream has significant jitter that cause glitches in voice sound.
// we left 40ms buffer to mitigate this effect and process stream in realtime with constant sample rate
static const int mic_latency_ms = 40;

// process frame size must be equal to FMOD frame size for smooth voice playing
static const int process_frame_size = 1024;

static const int process_frames_count = 4;


class VoiceFeedback
{
public:
  VoiceFeedback(unsigned int sample_rate, FMOD_SOUND_FORMAT sample_format, unsigned int frame_length_samples);

  ~VoiceFeedback();
  bool init(FMOD::System *fmod_sys, FMOD::ChannelGroup *channel_group);
  void shutdown();

  void onSoundData(int samplerate_in, const float *inbuffer, unsigned int nsamples, int nchannels, float *outbuffer);

  void lockFrame(void *&ptr1, unsigned &len);
  void unlockFrame();

private:
  int samplerateOut = 0;

  SpeexResamplerState *resampler = nullptr;
  FMOD::DSP *captureDSP = nullptr;
  FMOD::ChannelGroup *channelGroup = nullptr;

  eastl::vector<float> floatData;
  eastl::vector<int16_t> pcm16Data;
  eastl::vector<int16_t> outputFrame;

  unsigned int frameSize = 0;
  unsigned int nextFrame = 0;
  unsigned int nextFrameRead = 0;
  const unsigned int framesCount = 4;
  unsigned int sampleSize = 0;

  WinCritSec mutex;
};

// This callback is called from FMOD's update thread.
// So we can perform CPU-greedy tasks here as downsampling and mixing
// But if it (unlikely) will overload FMOD thread this tasks should be moved to other threads
// using job manager
static FMOD_RESULT F_CALL dsp_read(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels,
  int *outchannels)
{
  G_UNUSED(outchannels);

  // Unfortunately current (v 1.10) FMOD does not allow to create DSP blocks without sink.
  // Thus this pass-through copy must be performed
  memcpy(outbuffer, inbuffer, inchannels * length * sizeof(float));

  int samplerate = 0;
  FMOD_RESULT res = dsp_state->functions->getsamplerate(dsp_state, &samplerate);

  if (res == FMOD_OK)
  {
    void *userData = nullptr;
    res = dsp_state->functions->getuserdata(dsp_state, &userData);
    if (res == FMOD_OK && userData)
      static_cast<VoiceFeedback *>(userData)->onSoundData(samplerate, inbuffer, length, inchannels, outbuffer);
  }

  return FMOD_OK;
}

VoiceFeedback::VoiceFeedback(unsigned int sample_rate, FMOD_SOUND_FORMAT sample_format, unsigned int frame_length_samples) :
  samplerateOut(sample_rate), sampleSize(get_bytes_from_format(sample_format))
{
  G_UNUSED(sample_format);
  frameSize = frame_length_samples;
  outputFrame.resize(frame_length_samples * framesCount);
}

bool VoiceFeedback::init(FMOD::System *fmod_sys, FMOD::ChannelGroup *channel_group)
{
  FMOD_DSP_DESCRIPTION dspdescr;
  memset(&dspdescr, 0, sizeof(dspdescr));
  dspdescr.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
  strcpy(dspdescr.name, "voice echo feedback");
  dspdescr.numinputbuffers = 1;
  dspdescr.numoutputbuffers = 1;
  dspdescr.read = dsp_read;
  dspdescr.userdata = this;

  G_ASSERT(channel_group);
  channelGroup = channel_group;

  FMOD_RESULT fmodRes;
  fmodRes = fmod_sys->createDSP(&dspdescr, &captureDSP);
  CHECK_FMOD_RETURN_VAL(fmodRes, "createDSP", false);
  fmodRes = channelGroup->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, captureDSP);
  CHECK_FMOD_RETURN_VAL(fmodRes, "addDSP", false);
  captureDSP->setActive(true);
  return true;
}

void VoiceFeedback::shutdown()
{
  if (captureDSP)
  {
    FMOD::System *fmodSys = nullptr;
    captureDSP->getSystemObject(&fmodSys);
    fmodSys->lockDSP();
    captureDSP->setUserData(nullptr);
    if (channelGroup)
      channelGroup->removeDSP(captureDSP);
    captureDSP->release();
    fmodSys->unlockDSP();
  }
}

VoiceFeedback::~VoiceFeedback()
{
  if (resampler)
    speex_resampler_destroy(resampler);
}

void VoiceFeedback::onSoundData(int samplerate_in, const float *inbuffer, unsigned int nsamples, int nchannels, float *outbuffer)
{
  G_UNUSED(outbuffer);
  if (samplerate_in != samplerateOut && !resampler)
  {
    resampler = speex_resampler_init(nchannels, samplerate_in, samplerateOut, 5, nullptr);
    speex_resampler_set_output_stride(resampler, 1);
    G_ASSERT_RETURN(resampler, );
  }

  unsigned int nsamplesOut = reRate(nsamples, samplerate_in, samplerateOut);

  int channelsnR;
  const float *resampledBuffer = nullptr;
  if (resampler)
  {
    floatData.resize(nsamplesOut);
    resampledBuffer = floatData.data();
    channelsnR = 1;
    speex_resampler_reset_mem(resampler);
    speex_resampler_process_float(resampler, 0, inbuffer, &nsamples, floatData.data(), &nsamplesOut);
  }
  else
  {
    resampledBuffer = inbuffer;
    nsamplesOut = nsamples;
    channelsnR = nchannels;
  }

  size_t bufSize = pcm16Data.size();
  pcm16Data.resize(bufSize + nsamplesOut);

  for (unsigned i = 0; i < nsamplesOut; ++i)
  {
    float sample = resampledBuffer[i * channelsnR];
    pcm16Data[bufSize + i] = clamp<int>(sample * INT16_MAX, INT16_MIN, INT16_MAX);
  }

  if (pcm16Data.size() >= frameSize)
  {
    WinAutoLock lock(mutex);
    outputFrame.resize(frameSize * framesCount);
    memcpy(&outputFrame[nextFrame * frameSize], pcm16Data.data(), frameSize * sample_size);
    nextFrame = (nextFrame + 1) % framesCount;
    int tailSize = pcm16Data.size() - frameSize;
    if (tailSize > 0)
    {
      memmove(&pcm16Data[0], &pcm16Data[frameSize], tailSize * sample_size);
      pcm16Data.resize(tailSize);
    }
  }
}

void VoiceFeedback::lockFrame(void *&ptr1, unsigned &len)
{
  mutex.lock();
  ptr1 = &outputFrame[nextFrameRead * frameSize];
  len = frameSize * sampleSize;
}

void VoiceFeedback::unlockFrame()
{
  nextFrameRead = (nextFrameRead + 1) % framesCount;
  mutex.unlock();
}

class VoiceCommunicatorImpl : public VoiceCommunicator
{
public:
  VoiceCommunicatorImpl(IVoiceChannel *vchan, SoundSettings const &settings);
  ~VoiceCommunicatorImpl();

  bool init() override;

  void updateSoundSettings(SoundSettings const &settings) override;

  void enableRecording() override;
  void disableRecording() override;
  bool isRecordingEnabled() const override { return isRecording; }

  void process() override;

private:
  bool readMicData(FMOD::System *fmod_sys);
  void updateTestSound(FMOD::System *fmodSys);
  void updateVoiceSound(FMOD::System *fmodSys);
  void preprocessMicData();

  bool startRecord();
  void stopRecord();

  bool createRecordSound(FMOD::System *fmod_sys, unsigned int sample_rate);

private:
  FMOD::ChannelGroup *masterChannelFroup = nullptr;
  FMOD::ChannelGroup *voiceChannelGroup = nullptr;
  fmod_ptr<FMOD::Sound> fakeMic;
  fmod_ptr<FMOD::Sound> tsSound;
  fmod_ptr<FMOD::Sound> playbackSound;
  fmod_ptr<FMOD::Sound> recordSound;

  FMOD_CREATESOUNDEXINFO record_sound_info;

  FMOD::Channel *tsChannel = nullptr;
  FMOD::Channel *playbackChannel = nullptr;

  IVoiceChannel *voiceChannel;

  unsigned int processSamplerate = 16000;

  unsigned int chanFrameSize = 0;
  unsigned int lastRecordPos = 0;

  int tsLastFilledFrame = -1;
  int playbackLastFilledFrame = -1;

  SoundSettings soundSettings;
  bool isRecording = false;
  bool isPortAttached = false;

  eastl::unique_ptr<VoiceFeedback> feedback;

  eastl::vector<int16_t> recordBuf;
  eastl::vector<int16_t> recordBufNoEcho;
  eastl::vector<int16_t> recordBufNative;
  eastl::vector<int16_t> voiceOutBuf;

  bool fillingRecordBuf = true;
  int currentRecordDevId = -1;

  SpeexEchoState_ *speexEchoSt = nullptr;
  SpeexPreprocessState_ *speexPPSt = nullptr;
  SpeexResamplerState_ *micResampler = nullptr;
};

bool VoiceCommunicatorImpl::init()
{
  if (!sndsys::is_inited())
    return false;
  FMOD::System *fmodSys = sndsys::fmodapi::get_system();
  G_ASSERT_RETURN(voiceChannel, false);
  processSamplerate = voiceChannel->getSampleRate();
  chanFrameSize = voiceChannel->getFrameSamples();

  FMOD_RESULT fmodRes;

  fmodRes = fmodSys->getMasterChannelGroup(&masterChannelFroup);
  CHECK_FMOD_RETURN_VAL(fmodRes, "getMasterChannelGroup", false);

  FMOD_CREATESOUNDEXINFO exinfo;
  memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
  exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
  exinfo.numchannels = 1;
  exinfo.defaultfrequency = processSamplerate;
  exinfo.format = process_sample_format;
  exinfo.length = process_frame_size * process_frames_count * sample_size;

  FMOD::Sound *sound = nullptr;
  fmodRes = fmodSys->createSound(NULL, FMOD_2D | FMOD_OPENUSER | FMOD_LOOP_NORMAL, &exinfo, &sound);
  CHECK_FMOD_RETURN_VAL(fmodRes, "failed to create test sound", false);
  tsSound.reset(sound);
  debug("[VC] create record test sound: sr %u, ns %d, nb %d", processSamplerate, exinfo.length / 2, exinfo.length);

  exinfo.length = chanFrameSize * process_frames_count * sample_size;
  fmodRes = fmodSys->createSound(NULL, FMOD_2D | FMOD_OPENUSER | FMOD_LOOP_NORMAL, &exinfo, &sound);
  CHECK_FMOD_RETURN_VAL(fmodRes, "failed to create playback sound", false);
  debug("[VC] create playback sound: sr %u, ns %d, nb %d", processSamplerate, exinfo.length / 2, exinfo.length);
  playbackSound.reset(sound);

  feedback.reset(new VoiceFeedback(processSamplerate, process_sample_format, process_frame_size));
  if (!feedback->init(fmodSys, masterChannelFroup))
  {
    debug("[VC] failed to initialize voice feedback");
    return false;
  }

  recordBuf.resize(process_frame_size);
  return true;
}

VoiceCommunicatorImpl::VoiceCommunicatorImpl(IVoiceChannel *vchan, SoundSettings const &ss) : voiceChannel(vchan), soundSettings(ss)
{
  memset(&record_sound_info, 0, sizeof(record_sound_info));

  debug("[VC] Voice playback sample rate %uhz, buffer size %u samples", processSamplerate, chanFrameSize);

  speexEchoSt = speex_echo_state_init(process_frame_size, process_frame_size * 30);
  speex_echo_ctl((SpeexEchoState *)speexEchoSt, SPEEX_ECHO_SET_SAMPLING_RATE, &processSamplerate);

  speexPPSt = speex_preprocess_state_init(process_frame_size, processSamplerate);
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_ECHO_STATE, speexEchoSt);

  int val = 1;
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_DENOISE, &val);
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_DEREVERB, &val);

  val = -30;
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &val);
}

VoiceCommunicatorImpl::~VoiceCommunicatorImpl()
{
  if (feedback)
    feedback->shutdown();
  if (speexEchoSt)
    speex_echo_state_destroy(speexEchoSt);
  if (speexPPSt)
    speex_preprocess_state_destroy(speexPPSt);
  if (micResampler)
    speex_resampler_destroy(micResampler);
}

bool VoiceCommunicatorImpl::readMicData(FMOD::System *fmodSys)
{
  if (!isRecording || !soundSettings.micEnabled)
    return false;

  if (!recordSound.get())
    return false;

  if (soundSettings.recordDeviceId < 0)
    return true;

  int recordDev = soundSettings.recordDeviceId;

  if (recordDev != currentRecordDevId)
  {
    fillingRecordBuf = true;
    currentRecordDevId = recordDev;
    lastRecordPos = 0;
    eastl::fill(recordBuf.begin(), recordBuf.end(), 0);
  }

  unsigned int micRate = record_sound_info.defaultfrequency;
  unsigned int curPos = 0;
  FMOD_RESULT fmodRes = fmodSys->getRecordPosition(recordDev, &curPos);
  if (fmodRes != FMOD_OK)
    return false;

  if (curPos == lastRecordPos)
    return false;

  unsigned int recordSamples = 0;
  fmodRes = recordSound->getLength(&recordSamples, FMOD_TIMEUNIT_PCM);

  int bufSamples = curPos - lastRecordPos;
  if (bufSamples <= 0)
    bufSamples += recordSamples;

  if (samplesToMs(bufSamples, micRate) < mic_latency_ms && fillingRecordBuf)
    return true;
  fillingRecordBuf = false;
  int frameSize = 0;

  if (micRate == processSamplerate)
    frameSize = process_frame_size;
  else
    frameSize = reRate(process_frame_size, processSamplerate, micRate);

  if (bufSamples < frameSize)
    return false;

  int16_t *outBuf;
  if (micRate == processSamplerate)
    outBuf = recordBuf.data();
  else
  {
    recordBufNative.resize(frameSize);
    outBuf = recordBufNative.data();
  }

  int16_t *ptr1;
  int16_t *ptr2;
  unsigned int len1;
  unsigned int len2;
  fmodRes = recordSound->lock(lastRecordPos * sample_size, frameSize * sample_size, (void **)&ptr1, (void **)&ptr2, &len1, &len2);
  CHECK_FMOD_RETURN_VAL(fmodRes, "failed to lock record buffer", false);
  memcpy(outBuf, ptr1, len1);
  if (ptr2)
    memcpy(outBuf + len1 / sample_size, ptr2, len2); // -V609
  recordSound->unlock(ptr1, ptr2, len1, len2);

  lastRecordPos += frameSize;
  if (lastRecordPos >= recordSamples)
    lastRecordPos -= recordSamples;

  if (micRate != processSamplerate)
  {
    if (!micResampler)
      micResampler = speex_resampler_init(1, micRate, processSamplerate, 5, nullptr);
    else
    {
      uint32_t inRate, outRate;
      speex_resampler_get_rate(micResampler, &inRate, &outRate);
      if (inRate != micRate || outRate != processSamplerate)
        speex_resampler_set_rate(micResampler, micRate, processSamplerate);
    }
    uint32_t inLen = recordBufNative.size();
    uint32_t outLen = recordBuf.size();
    speex_resampler_process_int(micResampler, 0, recordBufNative.data(), &inLen, recordBuf.data(), &outLen);
  }

  return true;
}

void VoiceCommunicatorImpl::updateTestSound(FMOD::System *fmodSys)
{
  bool isPlaying = false;
  if (tsChannel)
    tsChannel->isPlaying(&isPlaying);
  int nextFillFrame = 0;

  if (isPlaying)
  {
    unsigned int curPlayPos = 0;
    tsChannel->getPosition(&curPlayPos, FMOD_TIMEUNIT_PCM);
    int curPlayFrame = curPlayPos / process_frame_size;
    nextFillFrame = (curPlayFrame + 1) % process_frames_count;
  }
  else
  {
    nextFillFrame = 0;
  }

  if (nextFillFrame == tsLastFilledFrame)
    return;

  tsLastFilledFrame = nextFillFrame;
  int16_t *ptr1;
  int16_t *ptr2;
  unsigned int len1;
  unsigned int len2;
  tsSound->lock(nextFillFrame * process_frame_size * sample_size, process_frame_size * sample_size, (void **)&ptr1, (void **)&ptr2,
    &len1, &len2);
  memcpy(ptr1, recordBufNoEcho.data(), process_frame_size * sample_size);
  tsSound->unlock(ptr1, ptr2, len1, len2);

  if (!isPlaying)
  {
    fmodSys->playSound(tsSound.get(), voiceChannelGroup, 0, &tsChannel);
    tsChannel->setVolume(soundSettings.soundEnabled ? soundSettings.playbackVolume : 0.0f);
  }
}

void VoiceCommunicatorImpl::updateVoiceSound(FMOD::System *fmodSys)
{
  bool isPlaying = false;
  if (playbackChannel)
    playbackChannel->isPlaying(&isPlaying);
  int nextFillFrame = 0;

  if (isPlaying)
  {
    unsigned int curPlayPos = 0;
    playbackChannel->getPosition(&curPlayPos, FMOD_TIMEUNIT_PCM);
    int curPlayFrame = curPlayPos / chanFrameSize;
    nextFillFrame = (curPlayFrame + 1) % process_frames_count;
  }
  else
  {
    nextFillFrame = 0;
  }

  if (nextFillFrame == playbackLastFilledFrame)
    return;

  playbackLastFilledFrame = nextFillFrame;
  unsigned chanFrameBytes = chanFrameSize * sample_size;
  int16_t *ptr1;
  int16_t *ptr2;
  unsigned int len1;
  unsigned int len2;
  playbackSound->lock(nextFillFrame * chanFrameBytes, chanFrameBytes, (void **)&ptr1, (void **)&ptr2, &len1, &len2);
  G_ASSERT_RETURN(len1 == chanFrameBytes, );
  G_ASSERT_RETURN(ptr2 == nullptr, );

  voiceChannel->receiveAudioFrame(ptr1, chanFrameSize);
  playbackSound->unlock(ptr1, ptr2, len1, len2);

  if (!isPlaying)
  {
    fmodSys->playSound(playbackSound.get(), voiceChannelGroup, 0, &playbackChannel);
    playbackChannel->setVolume(soundSettings.soundEnabled ? soundSettings.playbackVolume : 0.0f);
  }
}

void VoiceCommunicatorImpl::preprocessMicData()
{
#if defined(DEBUG_SND_PROCESSING)
  uint64_t level_input = 0;
  uint64_t level_output = 0;
  uint64_t level_fb = 0;
  uint64_t level_delta = 0;
#endif
  recordBufNoEcho.resize(recordBuf.size());

  if (fabs(1.0 - soundSettings.recordVolume) > 0.01)
    for (unsigned i = 0; i < recordBuf.size(); ++i)
      recordBuf[i] *= soundSettings.recordVolume;

  void *fbData;
  unsigned fbLength;
  feedback->lockFrame(fbData, fbLength);

#if defined(DEBUG_SND_PROCESSING)
  for (unsigned i = 0; i < process_frame_size; ++i)
  {
    int16_t sample = ((int16_t *)fbData)[i];
    level_fb += sample * sample;
  }
#endif
  speex_echo_cancellation(speexEchoSt, recordBuf.data(), (const int16_t *)fbData, recordBufNoEcho.data());
  feedback->unlockFrame();

  speex_preprocess_run(speexPPSt, recordBufNoEcho.data());

#if defined(DEBUG_SND_PROCESSING)
  for (unsigned i = 0; i < process_frame_size; ++i)
  {
    int16_t so = recordBufNoEcho[i];
    int16_t si = recordBuf[i];
    level_output += so * so;
    level_delta += (so - si) * (so - si);
    level_input += si * si;
  }

  float li = sqrtf(level_input) / process_frame_size;
  float lo = sqrtf(level_output) / process_frame_size;
  float lfb = sqrtf(level_fb) / process_frame_size;
  float ld = sqrtf(level_delta) / process_frame_size;
  debug("li %.2f lfb %.2f lo %.2f ld %.2f", li, lfb, lo, ld);
#endif
}

void VoiceCommunicatorImpl::process()
{
  if (!sndsys::is_inited())
    return;
  FMOD::System *fmodSys = sndsys::fmodapi::get_system();
  if (isRecording && soundSettings.recordDeviceId >= 0)
  {
    bool isRealRecording = false;
    fmodSys->isRecording(soundSettings.recordDeviceId, &isRealRecording);
    if (!isRealRecording)
    {
      if (!startRecord())
        return;
    }
  }

  const bool hasNewRecFrame = readMicData(fmodSys);

  if (hasNewRecFrame)
    preprocessMicData();

  if (soundSettings.voiceTest)
    updateTestSound(fmodSys);

  if (voiceChannel)
  {
    if (hasNewRecFrame)
    {
      if (process_frame_size % chanFrameSize == 0)
      {
        voiceChannel->sendAudioFrame(recordBufNoEcho.data(), recordBufNoEcho.size());
      }
      else
      {
        voiceOutBuf.insert(voiceOutBuf.end(), recordBufNoEcho.begin(), recordBufNoEcho.end());
        unsigned int numSendFrames = voiceOutBuf.size() / chanFrameSize;
        unsigned int sentSamples = numSendFrames * chanFrameSize;
        for (unsigned i = 0; i < numSendFrames; ++i)
          voiceChannel->sendAudioFrame(&voiceOutBuf.data()[i * chanFrameSize], chanFrameSize);
        if (sentSamples > 0)
          voiceOutBuf.erase(voiceOutBuf.begin(), voiceOutBuf.begin() + sentSamples);
      }
    }

    updateVoiceSound(fmodSys);
  }
}

void VoiceCommunicatorImpl::enableRecording()
{
  if (!isRecording)
  {
    isRecording = true;
    startRecord();
  }
}

void VoiceCommunicatorImpl::disableRecording()
{
  if (isRecording)
  {
    isRecording = false;
    stopRecord();
  }
}

void VoiceCommunicatorImpl::updateSoundSettings(SoundSettings const &settings)
{
  if (!sndsys::is_inited())
    return;

  if ((settings.playbackVolume != soundSettings.playbackVolume) || (settings.soundEnabled != soundSettings.soundEnabled))
  {
    if (tsChannel)
      tsChannel->setVolume(settings.soundEnabled ? settings.playbackVolume : 0.0f);
    if (playbackChannel)
      playbackChannel->setVolume(settings.soundEnabled ? settings.playbackVolume : 0.0f);
  }

  soundSettings.micEnabled = settings.micEnabled;
  soundSettings.soundEnabled = settings.soundEnabled;
  soundSettings.playbackVolume = settings.playbackVolume;
  soundSettings.recordVolume = settings.recordVolume;

  if (soundSettings.recordDeviceId != settings.recordDeviceId)
  {
    stopRecord();
    soundSettings.recordDeviceId = settings.recordDeviceId;
    if (settings.recordDeviceId != -1)
      startRecord();
  }
}

bool VoiceCommunicatorImpl::createRecordSound(FMOD::System *fmod_sys, unsigned int sample_rate)
{
  int nsamples = sample_rate * mic_buf_duration_ms / 1000;
  int nbytes = nsamples * sample_size;

  if (!recordSound || record_sound_info.defaultfrequency != (int)sample_rate || record_sound_info.format != process_sample_format ||
      record_sound_info.length < nbytes)
  {
    FMOD_CREATESOUNDEXINFO exinfo = record_sound_info;
    exinfo.defaultfrequency = sample_rate;
    exinfo.format = process_sample_format;
    exinfo.length = nbytes;
    exinfo.numchannels = 1;
    exinfo.cbsize = sizeof(exinfo);

    debug("[VC] Create record sound: sr %u, ns %d, nb %d", sample_rate, nsamples, nbytes);
    FMOD::Sound *sound = nullptr;
    FMOD_RESULT fmodRes = fmod_sys->createSound(nullptr, FMOD_2D | FMOD_OPENUSER | FMOD_CREATESAMPLE, &exinfo, &sound);
    CHECK_FMOD_RETURN_VAL(fmodRes, "Failed to create record sound", false);
    record_sound_info = exinfo;
    recordSound.reset(sound);
  }
  return true;
}

bool VoiceCommunicatorImpl::startRecord()
{
  if (!sndsys::is_inited() || soundSettings.recordDeviceId < 0)
    return false;
  FMOD::System *fmodSys = sndsys::fmodapi::get_system();

  int systemrate = 0;
  FMOD_RESULT fmodRes;
  fmodRes = fmodSys->getRecordDriverInfo(soundSettings.recordDeviceId, nullptr, 0, nullptr, &systemrate, nullptr, nullptr, nullptr);

  if (fmodRes != FMOD_OK || systemrate == 0)
    return false;

  debug("[VC] Start record");
  if (createRecordSound(fmodSys, systemrate))
  {
    fmodRes = fmodSys->recordStart(soundSettings.recordDeviceId, recordSound.get(), true);
    CHECK_FMOD_RETURN_VAL(fmodRes, "Failed to start record", false);
  }
  return true;
}

void VoiceCommunicatorImpl::stopRecord()
{
  if (!sndsys::is_inited() || soundSettings.recordDeviceId < 0)
    return;
  debug("[VC] Stop record");
  FMOD::System *fmodSys = sndsys::fmodapi::get_system();
  fmodSys->recordStop(soundSettings.recordDeviceId);
  isRecording = false;
}

eastl::unique_ptr<VoiceCommunicator> create_voice_communicator(IVoiceChannel *vchan, SoundSettings const &settings)
{
  return eastl::make_unique<VoiceCommunicatorImpl>(vchan, settings);
}

} // namespace voicechat
