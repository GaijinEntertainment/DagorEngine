// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "voiceProcessor.h"
#include "rateHelpers.h"
#include "voiceSwitchVar.h"
#include <debug/dag_debug.h>
#include <math/dag_mathBase.h>
#include <perfMon/dag_cpuFreq.h>
#include <EASTL/algorithm.h>

#if defined(USE_RNNOISE_DENOISER)
#include <rnnoise.h>
#endif

static VoiceSwitchVar denoise_enabled("voice.denoise_enabled", true);
static VoiceSwitchVar preprocess_enabled("voice.preprocess_enabled", true);

namespace voicechat
{

#if defined(USE_RNNOISE_DENOISER)
// RNN supports only 48 khz sample rate
static const int sample_rate = 48000;
#else
static const int sample_rate = 32000;
#endif

// WebRTC operates only with 10ms frames
static const int frame_size = sample_rate / 100; // 10ms

// frames under -50 dBFS are mostly denoised silence, where the level ratio of the gain stage is rounding noise
static const double gain_measure_min_power = 32768.0 * 32768.0 * 1e-5;

void SpeexResamplerDeleter::operator()(SpeexResamplerState *resampler) const { speex_resampler_destroy(resampler); }

static dag::ConstSpan<int16_t> resample(SpeexResamplerPtr &resampler, int rate_in, dag::ConstSpan<int16_t> pcm_in, int rate_out,
  dag::Span<int16_t> pcm_out)
{
  if (rate_in == rate_out)
    return pcm_in;

  const int quality = 7;
  if (!resampler)
  {
    resampler.reset(speex_resampler_init(1, rate_in, rate_out, quality, nullptr));
    G_ASSERT_RETURN(resampler, {});
  }
  spx_uint32_t inLen = pcm_in.size();
  spx_uint32_t outLen = pcm_out.size();
  speex_resampler_process_int(resampler.get(), 0, pcm_in.data(), &inLen, pcm_out.data(), &outLen);
  G_ASSERT(inLen == pcm_in.size());
  return pcm_out.first(outLen);
}


static void reset_resampler(const SpeexResamplerPtr &resampler)
{
  if (resampler)
    speex_resampler_reset_mem(resampler.get());
}

static double mean_power(dag::ConstSpan<int16_t> frame)
{
  double sum = 0;
  for (int16_t sample : frame)
    sum += double(sample) * sample;
  return sum / frame.size();
}

static MicLevel measure_level(dag::ConstSpan<int16_t> frame)
{
  const double fullScale = 32768.0;
  int peak = 0;
  double sum = 0;
  for (int16_t sample : frame)
  {
    peak = max(peak, abs(int(sample)));
    sum += double(sample) * sample;
  }
  MicLevel level;
  // constraint: the build assumes finite math, so log10 must not get 0
  if (peak > 0)
  {
    level.rmsDb = max(float(10 * log10(sum / frame.size() / (fullScale * fullScale))), mic_level_floor_db);
    level.peakDb = float(20 * log10(peak / fullScale));
  }
  return level;
}

static webrtc::scoped_refptr<webrtc::AudioProcessing> create_audio_processing(const webrtc::AudioProcessing::Config &config)
{
  webrtc::scoped_refptr<webrtc::AudioProcessing> ap = webrtc::AudioProcessingBuilder().Create();
  webrtc::ProcessingConfig procConf{{{sample_rate, 1}, {sample_rate, 1}, {sample_rate, 1}, {sample_rate, 1}}};
  if (auto res = ap->Initialize(procConf); res != webrtc::AudioProcessing::kNoError)
    debug("[VC] failed initialized WebRTC AP %d", res);
  ap->ApplyConfig(config);
  return ap;
}

int VoiceProcessor::captureRate() { return sample_rate; }

int VoiceProcessor::frameSamples() { return frame_size; }

static void set_pipeline(webrtc::AudioProcessing::Config &config)
{
  config.pipeline.maximum_internal_processing_rate = sample_rate;
  config.pipeline.multi_channel_capture = false;
  config.pipeline.multi_channel_render = false;
}

static webrtc::AudioProcessing::Config gain_config(const CaptureTuning &tuning)
{
  webrtc::AudioProcessing::Config config;
  set_pipeline(config);
  config.echo_canceller.enabled = false;
  config.noise_suppression.enabled = false;
  config.high_pass_filter.enabled = false;
  config.gain_controller1.enabled = false;
  config.gain_controller2.enabled = true;
  config.gain_controller2.input_volume_controller.enabled = false;
  config.gain_controller2.adaptive_digital.enabled = true;
  config.gain_controller2.adaptive_digital.initial_gain_db = tuning.agc2InitialGainDb;
  config.gain_controller2.adaptive_digital.max_gain_db = tuning.agc2MaxGainDb;
  config.capture_level_adjustment.enabled = false;
  return config;
}

VoiceProcessor::VoiceProcessor(int output_rate) :
  tuning(capture_tuning(&tuningGeneration)),
  speechGate(frame_size, tuning.vadPrerollFrames, tuning.vadHangoverFrames),
  speechDetector(tuning.vadProbThreshold, tuning.vadCloseThreshold, tuning.vadOnsetFrames, tuning.vadSmoothing)
{
  setOutputRates(make_span_const(&output_rate, 1));
  webrtc::AudioProcessing::Config echoConfig;
#if defined(USE_RNNOISE_DENOISER)
  denoiseSt = rnnoise_create(nullptr);
  G_ASSERT(rnnoise_get_frame_size() == frame_size);
  denoiseBuf.resize(frame_size);
  debug("[VC] use RNNoise denoiser");
  echoConfig.noise_suppression.enabled = false;
#else
  debug("[VC] use WebRTC denoiser");
  echoConfig.noise_suppression.enabled = true;
#endif

  set_pipeline(echoConfig);
  echoConfig.echo_canceller.enabled = true;
  echoConfig.echo_canceller.mobile_mode = false;
  echoConfig.gain_controller1.enabled = false;
  echoConfig.gain_controller2.enabled = false;
  echoConfig.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kHigh;
  echoConfig.high_pass_filter.enabled = true;
  echoConfig.capture_level_adjustment.enabled = false;
  echoAP = create_audio_processing(echoConfig);

  gainAP = create_audio_processing(gain_config(tuning));
}

void VoiceProcessor::applyTuningChange()
{
  const int current = tuning_generation();
  if (current == tuningGeneration)
    return;
  tuning = capture_tuning(&tuningGeneration);
  speechGate.setTiming(tuning.vadPrerollFrames, tuning.vadHangoverFrames);
  speechDetector.setTuning(tuning.vadProbThreshold, tuning.vadCloseThreshold, tuning.vadOnsetFrames, tuning.vadSmoothing);
  gainAP->ApplyConfig(gain_config(tuning));
}

VoiceProcessor::~VoiceProcessor()
{
#if defined(USE_RNNOISE_DENOISER)
  if (denoiseSt)
    rnnoise_destroy(denoiseSt);
#endif
}

void ProcessingStats::add(const ProcessingStats &other)
{
  framesProcessed += other.framesProcessed;
  gateFrames += other.gateFrames;
  gateDroppedFrames += other.gateDroppedFrames;
  echoFrames += other.echoFrames;
  framesWithoutEcho += other.framesWithoutEcho;
  apmErrors += other.apmErrors;
  timedFrames += other.timedFrames;
  chainUsec += other.chainUsec;
  chainMaxUsec = max(chainMaxUsec, other.chainMaxUsec);
  denoiseUsec += other.denoiseUsec;
  denoiseFrames += other.denoiseFrames;
  if (other.gainFrames > 0)
  {
    minGainDb = gainFrames ? min(minGainDb, other.minGainDb) : other.minGainDb;
    maxGainDb = gainFrames ? max(maxGainDb, other.maxGainDb) : other.maxGainDb;
    gainFrames += other.gainFrames;
  }
}

ProcessingStats VoiceProcessor::takeStats()
{
  ProcessingStats taken = stats;
  const SpeechGate::Counts gate = speechGate.takeCounts();
  taken.gateFrames = gate.sent + gate.dropped;
  taken.gateDroppedFrames = gate.dropped;
  stats = {};
  return taken;
}

void VoiceProcessor::countApmResult(int result, const char *stage)
{
  if (result == webrtc::AudioProcessing::kNoError)
    return;
  ++stats.apmErrors;
  debug("[VC] %s returned an error %d", stage, result);
}

void VoiceProcessor::countGain(double power_in, dag::ConstSpan<int16_t> out)
{
  if (power_in < gain_measure_min_power)
    return;
  const float gainDb = 10 * log10(max(mean_power(out), 1.0) / power_in);
  stats.minGainDb = stats.gainFrames ? min(stats.minGainDb, gainDb) : gainDb;
  stats.maxGainDb = stats.gainFrames ? max(stats.maxGainDb, gainDb) : gainDb;
  ++stats.gainFrames;
}

void VoiceProcessor::resetAdaptation()
{
  echoAP->Initialize();
  gainAP->Initialize();
}

void VoiceProcessor::startStream()
{
  speechGate.forceOpen();
  level = {};
  breakStream();
}

void VoiceProcessor::breakStream()
{
  for (Output &out : outputs)
  {
    reset_resampler(out.resampler);
    reset_resampler(out.rawResampler);
  }
}

void VoiceProcessor::setOutputRates(dag::ConstSpan<int> rates)
{
  G_ASSERT_RETURN(!rates.empty(), );
  eastl::vector<Output> next(rates.size());
  for (int i = 0; i < int(rates.size()); ++i)
  {
    auto kept = eastl::find_if(outputs.begin(), outputs.end(), [rate = rates[i]](const Output &out) { return out.rate == rate; });
    if (kept != outputs.end())
      next[i] = eastl::move(*kept);
    next[i].rate = rates[i];
  }
  outputs = eastl::move(next);
}

const VoiceProcessor::Output *VoiceProcessor::findOutput(int rate) const
{
  auto found = eastl::find_if(outputs.begin(), outputs.end(), [rate](const Output &out) { return out.rate == rate; });
  return found != outputs.end() ? &*found : nullptr;
}

dag::ConstSpan<int16_t> VoiceProcessor::output(int rate) const
{
  const Output *out = findOutput(rate);
  return out ? out->processed : dag::ConstSpan<int16_t>();
}

dag::ConstSpan<int16_t> VoiceProcessor::rawOutput(int rate) const
{
  const Output *out = findOutput(rate);
  return out ? out->raw : dag::ConstSpan<int16_t>();
}

dag::ConstSpan<int16_t> VoiceProcessor::process(dag::ConstSpan<int16_t> capture, float record_volume, EchoFramePop pop_echo,
  bool want_raw)
{
  const int64_t startRef = statsEnabled ? ref_time_ticks() : 0;
  dag::ConstSpan<int16_t> processed = processFrames(capture, record_volume, pop_echo);
  const bool rawStreamBreaks = !want_raw || (processed.empty() && !capture.empty());
  for (Output &out : outputs)
  {
    out.processed = out.raw = {};
    if (rawStreamBreaks)
      reset_resampler(out.rawResampler);
    if (processed.empty())
      continue;
    out.buf.resize(re_rate(processed.size(), sample_rate, out.rate));
    out.processed = resample(out.resampler, sample_rate, processed, out.rate, make_span(out.buf));
    if (want_raw)
    {
      out.rawBuf.resize(re_rate(capture.size(), sample_rate, out.rate));
      out.raw = resample(out.rawResampler, sample_rate, capture, out.rate, make_span(out.rawBuf));
    }
  }
  const int frames = int(capture.size()) / frame_size;
  if (statsEnabled && frames > 0)
  {
    const int callUsec = int(ref_time_delta_to_usec(ref_time_ticks() - startRef));
    stats.chainUsec += callUsec;
    // rounded up, so the mean never exceeds the max
    stats.chainMaxUsec = max(stats.chainMaxUsec, (callUsec + frames - 1) / frames);
    stats.timedFrames += frames;
  }
  return outputs[0].processed;
}

dag::ConstSpan<int16_t> VoiceProcessor::processFrames(dag::ConstSpan<int16_t> capture, float record_volume, EchoFramePop pop_echo)
{
  const int numFrames = capture.size() / frame_size;
  const bool preprocess = preprocess_enabled.load();
  bool lastFrameSent = true;
  processedBuf.resize(capture.size());
  dag::Span<int16_t> pcmResult = make_span(processedBuf.data(), processedBuf.size());
  webrtc::StreamConfig sconf = {sample_rate, 1};
#if defined(USE_RNNOISE_DENOISER)
  const bool denoise = denoise_enabled.load();
  gatedBuf.clear();
#endif

  for (int n = 0; n < numFrames; ++n)
  {
    applyTuningChange();
    dag::ConstSpan<int16_t> frameIn = capture.subspan(n * frame_size, frame_size);
    dag::Span<int16_t> frame = pcmResult.subspan(n * frame_size, frame_size);
    ++stats.framesProcessed;
    if (preprocess)
    {
      ++stats.echoFrames;
      bool echoFed = false;
      while (pop_echo(frame))
      {
        countApmResult(echoAP->ProcessReverseStream(frame.data(), sconf, sconf, frame.data()), "echo ProcessReverseStream");
        echoFed = true;
      }
      stats.framesWithoutEcho += !echoFed;
      echoAP->set_stream_delay_ms(0);
      countApmResult(echoAP->ProcessStream(frameIn.data(), sconf, sconf, frame.data()), "echo ProcessStream");
    }
    else
      mem_copy_from(frame, frameIn.data());
    for (int16_t &sample : frame)
      sample = clamp<int>(float(sample) * record_volume, INT16_MIN, INT16_MAX);

#if defined(USE_RNNOISE_DENOISER)
    float speechProb = 0.0f;
    if (denoise)
    {
      const int64_t denoiseStartRef = statsEnabled ? ref_time_ticks() : 0;
      // RNNNoise works with float samples in range INT16_MIN/INT16_MAX
      for (unsigned i = 0; i < frame.size(); ++i)
        denoiseBuf[i] = frame[i];
      // it is ok to have same buffer for input and output
      speechProb = rnnoise_process_frame(denoiseSt, &denoiseBuf[0], &denoiseBuf[0]);
      for (unsigned i = 0; i < frame.size(); ++i)
        frame[i] = denoiseBuf[i];
      if (statsEnabled)
      {
        stats.denoiseUsec += ref_time_delta_to_usec(ref_time_ticks() - denoiseStartRef);
        ++stats.denoiseFrames;
      }
    }
#endif

    if (preprocess)
    {
      const double powerIn = statsEnabled ? mean_power(frame) : 0.0;
      countApmResult(gainAP->ProcessStream(frame.data(), sconf, sconf, frame.data()), "gain ProcessStream");
      if (statsEnabled)
        countGain(powerIn, frame);
    }

#if defined(USE_RNNOISE_DENOISER)
    if (denoise)
      lastFrameSent = speechGate.process(frame, speechDetector.isSpeech(speechProb, speechGate.isOpen()), gatedBuf);
#endif
  }

  if (numFrames > 0)
  {
    level = measure_level(pcmResult.subspan((numFrames - 1) * frame_size, frame_size));
    level.gateOpen = lastFrameSent;
  }

#if defined(USE_RNNOISE_DENOISER)
  if (denoise)
    return make_span(gatedBuf);
#endif
  return pcmResult;
}

} // namespace voicechat
