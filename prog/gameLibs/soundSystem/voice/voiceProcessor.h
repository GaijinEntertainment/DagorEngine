// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "speex_headers.h"
#include "speechGate.h"
#include "voiceTuning.h"
#include <generic/dag_functionRef.h>
#include <generic/dag_span.h>
#include <soundSystem/voiceCommunicator.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <webrtc/api/audio/audio_processing.h>

struct DenoiseState;

namespace voicechat
{

// counts and times of the processing chain since the previous VoiceProcessor::takeStats
struct ProcessingStats
{
  int framesProcessed = 0;
  // the gate and the echo canceller can be off, so each count goes with the frames its stage saw
  int gateFrames = 0;
  int gateDroppedFrames = 0;
  int echoFrames = 0;
  int framesWithoutEcho = 0;
  int apmErrors = 0;
  // gain of the AGC2 stage, over the frames loud enough to measure it
  int gainFrames = 0;
  float minGainDb = 0;
  float maxGainDb = 0;
  // time of the whole process() call, resampling included, over the frames timed while the stats were on, the max as
  // the worst call per frame; RNNoise over the frames it ran in
  int timedFrames = 0;
  int64_t chainUsec = 0;
  int64_t denoiseUsec = 0;
  int chainMaxUsec = 0;
  int denoiseFrames = 0;

  void add(const ProcessingStats &other);
};

struct SpeexResamplerDeleter
{
  void operator()(SpeexResamplerState *resampler) const;
};
using SpeexResamplerPtr = eastl::unique_ptr<SpeexResamplerState, SpeexResamplerDeleter>;

// The mic processing chain: echo cancelling (with the WebRTC denoiser where RNNoise is absent), record volume,
// the RNNoise denoiser, gain control, the RNNoise speech gate and resampling to each output rate. It knows no FMOD
// and no thread.
class VoiceProcessor
{
public:
  // fills one far-end frame of frameSamples() at captureRate(); false when none is left
  using EchoFramePop = dag::FunctionRef<bool(dag::Span<int16_t> frame)>;

  static int captureRate();
  static int frameSamples();

  explicit VoiceProcessor(int output_rate);
  ~VoiceProcessor();
  int firstOutputRate() const { return outputs[0].rate; }
  // distinct rates, at least one, the first is the one process() returns; a kept rate keeps its stream
  void setOutputRates(dag::ConstSpan<int> rates);

  VoiceProcessor(const VoiceProcessor &) = delete;
  VoiceProcessor &operator=(const VoiceProcessor &) = delete;

  // the adaptation of echo cancelling and gain control starts over, as for a new input device
  void resetAdaptation();
  // a new capture stream starts: the gate opens and the resamplers restart
  void startStream();
  // the capture stream pauses: the resamplers restart
  void breakStream();
  void dropSpeechGatePreroll() { speechGate.dropPreroll(); }
  // the thread that calls process(); silence from startStream until the first processed frame
  MicLevel micLevel() const { return level; }

  // capture holds whole frames at captureRate(). Returns the processed audio at the first output rate, valid until
  // the next call.
  dag::ConstSpan<int16_t> process(dag::ConstSpan<int16_t> capture, float record_volume, EchoFramePop pop_echo, bool want_raw);
  // the last process() result at any output rate, empty for a rate not in the set; valid until the next call.
  // The raw output is the unprocessed capture, there only when want_raw was set and the processed output is not empty;
  // it can point into capture.
  dag::ConstSpan<int16_t> output(int rate) const;
  dag::ConstSpan<int16_t> rawOutput(int rate) const;

  ProcessingStats takeStats();
  // off: the gain and the chain time are not measured, since they cost work on each frame; the other counts always run
  void setStatsEnabled(bool enabled) { statsEnabled = enabled; }

private:
  struct Output
  {
    int rate = 0;
    SpeexResamplerPtr resampler;
    SpeexResamplerPtr rawResampler;
    eastl::vector<int16_t> buf;
    eastl::vector<int16_t> rawBuf;
    dag::ConstSpan<int16_t> processed;
    dag::ConstSpan<int16_t> raw;
  };

  const Output *findOutput(int rate) const;
  dag::ConstSpan<int16_t> processFrames(dag::ConstSpan<int16_t> capture, float record_volume, EchoFramePop pop_echo);
  void countApmResult(int result, const char *stage);
  void countGain(double power_in, dag::ConstSpan<int16_t> out);
  void applyTuningChange();

  // constraint: AEC3 needs linear audio, AGC2 denoised
  webrtc::scoped_refptr<webrtc::AudioProcessing> echoAP;
  webrtc::scoped_refptr<webrtc::AudioProcessing> gainAP;
  DenoiseState *denoiseSt = nullptr;
  // constraint: before tuning, speechGate and speechDetector, which the constructor builds from the loaded values
  int tuningGeneration = 0;
  CaptureTuning tuning;
  SpeechGate speechGate;
  SpeechDetector speechDetector;

  eastl::vector<int16_t> processedBuf;
  eastl::vector<float> denoiseBuf;
  eastl::vector<int16_t> gatedBuf;
  ProcessingStats stats;
  MicLevel level;
  eastl::vector<Output> outputs;

  bool statsEnabled = false;
};

} // namespace voicechat
