// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <EASTL/vector.h>

namespace voicechat
{

// turns the speech probability of each frame into the speech flag of the gate: the probability opens a closed gate after
// onset_frames frames at the open threshold, and its smoothed value keeps an open one at the close threshold.
// constraint: isSpeech advances the detector, so it takes each frame once, with the gate state before that frame
class SpeechDetector
{
public:
  SpeechDetector(float open_threshold, float close_threshold, int onset_frames, float smoothing)
  {
    setTuning(open_threshold, close_threshold, onset_frames, smoothing);
  }

  void setTuning(float open_threshold, float close_threshold, int onset_frames, float smoothing);
  bool isSpeech(float probability, bool gate_open);

private:
  float openThreshold;
  float closeThreshold;
  float smoothingWeight;
  int onsetFrames;
  float smoothed = 0.0f;
  int framesAbove = 0;
};

class SpeechGate
{
public:
  // frames that left the gate since the previous take; a frame in the preroll counts when it leaves
  struct Counts
  {
    int sent = 0;
    int dropped = 0;
  };

  SpeechGate(int frame_samples, int preroll_frames, int hangover_frames);

  void forceOpen();
  bool isOpen() const { return hangoverLeft > 0 || startPending; }
  void dropPreroll();
  // a longer preroll than the new one drops its oldest frames, a longer hangover left is cut to the new length
  void setTiming(int preroll_frames, int hangover_frames);
  // returns true when the frame is sent now, false when it goes to the preroll
  bool process(dag::ConstSpan<int16_t> frame, bool is_speech, eastl::vector<int16_t> &out);
  Counts takeCounts();

private:
  void pushPreroll(dag::ConstSpan<int16_t> frame);
  void trimPreroll();
  void flushPreroll(eastl::vector<int16_t> &out);

  const int frameSamples;
  int prerollSamples;
  int hangoverFrames;

  eastl::vector<int16_t> preroll;
  int hangoverLeft = 0;
  Counts counts;
  bool startPending = false;
};

} // namespace voicechat
