// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "speex_headers.h"
#include "voiceFeedback.h"
#include <EASTL/unique_ptr.h>
#include "helpers.h"
#include <webrtc/api/audio/audio_processing.h>

struct DenoiseState;

namespace voicechat
{

struct SoundSettings;

class VoiceRecorder
{
public:
  VoiceRecorder(int output_samplerate);
  ~VoiceRecorder();

  bool init(FMOD::System *fmod_sys, FMOD::ChannelGroup *channel_group);
  // Returns processed (denoised) audio. If out_raw is non-null AND raw output is enabled,
  // also writes raw (pre-denoise) audio to *out_raw. Both spans are valid until next call.
  // Must only be called from one thread (the VoiceCommunicator process thread).
  dag::ConstSpan<int16_t> readMicData(FMOD::System *fmod_sys, const SoundSettings &sound_settings,
    dag::ConstSpan<int16_t> *out_raw = nullptr);

  bool startRecord(FMOD::System *fmod_sys, const SoundSettings &sound_settings);
  void stopRecord(FMOD::System *fmod_sys, const SoundSettings &sound_settings);

private:
  bool createRecordSound(FMOD::System *fmod_sys);
  dag::Span<int16_t> postprocessFrame(dag::Span<int16_t> pcm_frame, const SoundSettings &sound_settings);
  dag::Span<int16_t> readFrames(FMOD::System *fmod_sys, const SoundSettings &sound_settings);

private:
  webrtc::scoped_refptr<webrtc::AudioProcessing> webrtcAP;

  fmod_ptr<FMOD::Sound> recordSound;
  unsigned recordSoundSamples = 0;
  FMOD_CREATESOUNDEXINFO recordSoundInfo;

  eastl::unique_ptr<VoiceFeedback> feedback;
  SpeexResamplerState *outputResampler = nullptr;

  DenoiseState *denoiseSt = nullptr;

  bool isRecording = false;
  int currentRecordDevId = -1;
  unsigned int recordPos = 0;

  eastl::vector<int16_t> PCMBuf1;
  eastl::vector<int16_t> PCMBuf2;
  eastl::vector<int16_t> outputBuf;
  eastl::vector<float> denoiseBuf;

  // Raw (pre-postprocess) audio capture and resampled output.
  eastl::vector<int16_t> rawCaptureBuf; // snapshot of PCMBuf1 before postprocessFrame
  eastl::vector<int16_t> rawOutputBuf;  // resampled to outputSamplerate
  SpeexResamplerState *rawOutputResampler = nullptr;
  volatile int rawOutputEnabled = 0;
  bool snapshotRawCapture = false; // set by readMicData, consumed by readFrames (same thread)

  int outputSamplerate = 0;
  int silenceStartMs = 0;

public:
  // Thread-safe toggle: written from worker thread, read from process thread.
  void setRawOutputEnabled(bool enabled);
};

} // namespace voicechat
