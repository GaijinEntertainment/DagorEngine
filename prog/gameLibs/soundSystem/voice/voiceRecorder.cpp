// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "voiceRecorder.h"
#include "helpers.h"
#include <soundSystem/voiceCommunicator.h>
#include <debug/dag_debug.h>
#include <util/dag_convar.h>
#include <perfMon/dag_cpuFreq.h>

#if defined(USE_RNNOISE_DENOISER)
#include <rnnoise.h>
#endif

static CONSOLE_BOOL_VAL("voice", denoise_enabled, true);
static CONSOLE_BOOL_VAL("voice", preprocess_enabled, true);

namespace voicechat
{

static const int sample_rate = 48000;
static const int frame_size = 480; // 10ms
static const FMOD_SOUND_FORMAT sample_format = FMOD_SOUND_FORMAT_PCM16;
static const int sample_size = get_bytes_from_format(sample_format);

#if defined(USE_RNNOISE_DENOISER)
static const float rnnoise_gain = 0.95f;
static const float rnnoise_vad_prob_threshold = 0.6;
static const float rnnoise_vad_silence_delay_ms = 300;
#endif

static dag::ConstSpan<int16_t> resample(SpeexResamplerState_ *&resampler, int rate_in, dag::ConstSpan<int16_t> pcm_in, int rate_out,
  dag::Span<int16_t> pcm_out)
{
  if (rate_in == rate_out)
    return pcm_in;

  // this naive downsampling give much better quality than complex
  // speex resampler
  if (rate_out < rate_in && rate_in % rate_out == 0)
  {
    int k = rate_in / rate_out;
    for (int i = 0; i < pcm_out.size(); ++i)
      pcm_out[i] = pcm_in[i * k];
    return pcm_out;
  }

  const int quality = 7;
  if (resampler == nullptr)
    resampler = speex_resampler_init(1, rate_in, rate_out, quality, nullptr);
  else
  {
    uint32_t inRate, outRate;
    speex_resampler_get_rate(resampler, &inRate, &outRate);
    if (inRate != rate_in || outRate != rate_out)
    {
      speex_resampler_set_rate(resampler, rate_in, rate_out);
      speex_resampler_reset_mem(resampler);
    }
  }
  spx_uint32_t inLen = pcm_in.size();
  spx_uint32_t outLen = pcm_out.size();
  speex_resampler_process_int(resampler, 0, pcm_in.data(), &inLen, pcm_out.data(), &outLen);
  G_ASSERT(inLen == pcm_in.size());
  return pcm_out.first(outLen);
}


VoiceRecorder::VoiceRecorder(int output_samplerate) : outputSamplerate(output_samplerate)
{
  memset(&recordSoundInfo, 0, sizeof(recordSoundInfo));
  speexEchoSt = speex_echo_state_init(frame_size, frame_size * 30); // 10ms ... 300ms
  int sampleRate = sample_rate;
  speex_echo_ctl(speexEchoSt, SPEEX_ECHO_SET_SAMPLING_RATE, &sampleRate);

  speexPPSt = speex_preprocess_state_init(frame_size, sample_rate);
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_ECHO_STATE, speexEchoSt);
  int val = 1;
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_DEREVERB, &val);
  val = -30;
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS, &val);

  val = 1;
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_AGC, &val);
  val = 16000;
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_AGC_TARGET, &val);


#if defined(USE_RNNOISE_DENOISER)
  denoiseSt = rnnoise_create(nullptr);
  G_ASSERT(rnnoise_get_frame_size() == frame_size);
  denoiseBuf.resize(frame_size);
  debug("[VC] use RNNoise denoiser");
#else
  val = 1;
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_DENOISE, &val);
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_VAD, &val);

  val = 70;
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_PROB_START, &val);

  val = 90;
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &val);

  val = -30;
  speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &val);
  debug("[VC] use speex denoiser");
#endif
}

VoiceRecorder::~VoiceRecorder()
{
  if (feedback)
    feedback->shutdown();
  if (speexEchoSt)
    speex_echo_state_destroy(speexEchoSt);
  if (speexPPSt)
    speex_preprocess_state_destroy(speexPPSt);
  if (outputResampler)
    speex_resampler_destroy(outputResampler);
#if defined(USE_RNNOISE_DENOISER)
  if (denoiseSt)
    rnnoise_destroy(denoiseSt);
#endif
}

dag::Span<int16_t> VoiceRecorder::readFrames(FMOD::System *fmod_sys, const SoundSettings &sound_settings)
{
  unsigned int curPos = 0;
  FMOD_RESULT fmodRes = fmod_sys->getRecordPosition(currentRecordDevId, &curPos);
  if (fmodRes != FMOD_OK)
    return {};
  if (curPos == recordPos)
    return {};

  int bufSamples = curPos - recordPos;
  if (bufSamples < 0)
    bufSamples += recordSoundSamples;

  if (bufSamples < frame_size)
    return {};

  int numFrames = bufSamples / frame_size;
  PCMBuf1.resize(frame_size * numFrames);
  dag::Span<int16_t> recordSource = {PCMBuf1.data(), (int)PCMBuf1.size()};

  {
    int16_t *ptr1;
    int16_t *ptr2;
    unsigned int len1;
    unsigned int len2;
    fmodRes =
      recordSound->lock(recordPos * sample_size, numFrames * frame_size * sample_size, (void **)&ptr1, (void **)&ptr2, &len1, &len2);
    CHECK_FMOD_RETURN_VAL(fmodRes, "failed to lock record buffer", {});
    memcpy(recordSource.data(), ptr1, len1);
    if (ptr2)
      memcpy(&recordSource[len1 / sample_size], ptr2, len2); // -V609
    recordSound->unlock(ptr1, ptr2, len1, len2);
    unsigned readSamples = (len1 + len2) / sample_size;
    recordPos = (recordPos + readSamples) % recordSoundSamples;
  }

  return postprocessFrame(recordSource, sound_settings);
}

dag::ConstSpan<int16_t> VoiceRecorder::readMicData(FMOD::System *fmod_sys, const SoundSettings &sound_settings)
{
  if (!isRecording || !sound_settings.micEnabled || !recordSound.get() || sound_settings.recordDeviceId < 0)
    return {};

  dag::Span<int16_t> pcmFrames = readFrames(fmod_sys, sound_settings);
  if (pcmFrames.empty())
    return {};
  int outputSamples = re_rate(pcmFrames.size(), sample_rate, outputSamplerate);
  outputBuf.resize(outputSamples);
  return resample(outputResampler, sample_rate, pcmFrames, outputSamplerate, make_span(outputBuf.data(), outputSamples));
}

dag::Span<int16_t> VoiceRecorder::postprocessFrame(dag::Span<int16_t> pcm_frames, const SoundSettings &sound_settings)
{
  if (pcm_frames.empty())
    return {};
  const int numFrames = pcm_frames.size() / frame_size;
#if defined(USE_RNNOISE_DENOISER)
  if (denoise_enabled)
  {
    float speechProb = 0.0f;
    for (int n = 0; n < numFrames; ++n)
    {
      dag::Span<int16_t> frame = pcm_frames.subspan(n * frame_size, frame_size);
      // rnnoise uses not floating point PCM with -1:1 range. It requires 16-bit PCM in floating-point represantation
      for (unsigned i = 0; i < frame.size(); ++i)
        denoiseBuf[i] = frame[i] * rnnoise_gain;
      // it is ok too have same buffer for input and output
      speechProb = max(speechProb, rnnoise_process_frame(denoiseSt, &denoiseBuf[0], &denoiseBuf[0]));
      for (unsigned i = 0; i < frame.size(); ++i)
        frame[i] = denoiseBuf[i];
    }
    if (speechProb < rnnoise_vad_prob_threshold)
    {
      int now = get_time_msec();
      if (silenceStartMs == 0)
        silenceStartMs = now;
      else if (now - silenceStartMs > rnnoise_vad_silence_delay_ms)
        return {};
    }
    else
      silenceStartMs = 0;
  }
#endif

  dag::Span<int16_t> pcmResult = pcm_frames;
  if (preprocess_enabled)
  {
    bool speech = false;
    PCMBuf2.resize(pcm_frames.size());
    pcmResult = make_span(PCMBuf2.data(), PCMBuf2.size());
    for (unsigned i = 0; i < numFrames; ++i)
    {
      dag::ConstSpan<int16_t> frameIn = pcm_frames.subspan(i * frame_size, frame_size);
      dag::Span<int16_t> frameOut = pcmResult.subspan(i * frame_size, frame_size);

      dag::ConstSpan<int16_t> echoSource = feedback->lockFrame();
      if (!echoSource.empty())
        speex_echo_cancellation(speexEchoSt, frameIn.data(), echoSource.data(), frameOut.data());
      feedback->unlockFrame();
      if (echoSource.empty())
        mem_copy_from(frameOut, frameIn.data());
      speech |= speex_preprocess_run(speexPPSt, frameOut.data()) == 1;
    }
    if (false)
    {
      int loudness = 0;
      speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_GET_AGC_LOUDNESS, &loudness);
      int gain = 0;
      speex_preprocess_ctl(speexPPSt, SPEEX_PREPROCESS_GET_AGC_GAIN, &gain);
      debug("[VC] loudness %d gain %d", loudness, gain);
    }
    if (!speech)
      pcmResult.reset();
  }

  if (!pcmResult.empty() && fabsf(1.0f - sound_settings.recordVolume) > 0.01f)
    for (int16_t &sample : pcmResult)
      sample = clamp<int>(float(sample) * sound_settings.recordVolume, INT16_MIN, INT16_MAX);
  return pcmResult;
}

bool VoiceRecorder::init(FMOD::System *fmod_sys, FMOD::ChannelGroup *channel_group)
{
  feedback.reset(new VoiceFeedback(sample_rate, sample_format, frame_size));
  if (!feedback->init(fmod_sys, channel_group))
  {
    debug("[VC] failed to initialize voice feedback");
    return false;
  }
  return true;
}

bool VoiceRecorder::createRecordSound(FMOD::System *fmod_sys)
{
  if (!recordSound || recordSoundInfo.format != sample_format)
  {
    unsigned dspBufLen = 0;
    int dspBufsNum = 0;
    fmod_sys->getDSPBufferSize(&dspBufLen, &dspBufsNum);
    const int nsamples = dspBufLen * dspBufsNum;
    const int nbytes = nsamples * sample_size;

    FMOD_CREATESOUNDEXINFO exinfo = recordSoundInfo;
    exinfo.defaultfrequency = sample_rate;
    exinfo.format = sample_format;
    exinfo.length = nbytes;
    exinfo.numchannels = 1;
    exinfo.cbsize = sizeof(exinfo);

    debug("[VC] Create record sound: sr %u, ns %d, nb %d", sample_rate, nsamples, nbytes);
    FMOD::Sound *sound = nullptr;
    FMOD_RESULT fmodRes = fmod_sys->createSound(nullptr, FMOD_2D | FMOD_OPENUSER | FMOD_CREATESAMPLE, &exinfo, &sound);
    CHECK_FMOD_RETURN_VAL(fmodRes, "Failed to create record sound", false);
    recordSoundInfo = exinfo;
    recordSound.reset(sound);
    fmodRes = recordSound->getLength(&recordSoundSamples, FMOD_TIMEUNIT_PCM);
    CHECK_FMOD_RETURN_VAL(fmodRes, "Sound::getLength", false);
    G_ASSERT(recordSoundSamples == nsamples);
  }

  return true;
}

bool VoiceRecorder::startRecord(FMOD::System *fmod_sys, const SoundSettings &sound_settings)
{
  debug("[VC] Start record on device %d", sound_settings.recordDeviceId);
  FMOD_RESULT fmodRes;
  FMOD_DRIVER_STATE state;
  fmodRes = fmod_sys->getRecordDriverInfo(sound_settings.recordDeviceId, nullptr, 0, nullptr, nullptr, nullptr, nullptr, &state);

  if (fmodRes != FMOD_OK || ((state & FMOD_DRIVER_STATE_CONNECTED) == 0))
  {
    debug("[VC] Record start failed. Device not connected");
    return false;
  }

  currentRecordDevId = sound_settings.recordDeviceId;
  if (createRecordSound(fmod_sys))
  {
    fmodRes = fmod_sys->recordStart(sound_settings.recordDeviceId, recordSound.get(), true);
    CHECK_FMOD_RETURN_VAL(fmodRes, "Failed to start record", false);
    fmodRes = fmod_sys->getRecordPosition(sound_settings.recordDeviceId, &recordPos);
    CHECK_FMOD_RETURN_VAL(fmodRes, "getRecordPosition", false);
    feedback->enable();
    isRecording = true;
  }
  return true;
}

void VoiceRecorder::stopRecord(FMOD::System *fmod_sys, const SoundSettings &sound_settings)
{
  debug("[VC] Stop record");
  fmod_sys->recordStop(sound_settings.recordDeviceId);
  isRecording = false;
  feedback->disable();
  speex_echo_state_reset(speexEchoSt);
  outputBuf.clear();
}

} // namespace voicechat
