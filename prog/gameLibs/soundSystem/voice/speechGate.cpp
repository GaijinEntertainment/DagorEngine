// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "speechGate.h"
#include <debug/dag_assert.h>
#include <EASTL/algorithm.h>

namespace voicechat
{

static void fade_in(dag::Span<int16_t> samples)
{
  const int n = samples.size();
  for (int i = 0; i < n; ++i)
    samples[i] = samples[i] * (i + 1) / n;
}

static void fade_out(dag::Span<int16_t> samples)
{
  const int n = samples.size();
  for (int i = 0; i < n; ++i)
    samples[i] = samples[i] * (n - 1 - i) / n;
}

void SpeechDetector::setTuning(float open_threshold, float close_threshold, int onset_frames, float smoothing)
{
  openThreshold = open_threshold;
  closeThreshold = close_threshold;
  onsetFrames = onset_frames;
  smoothingWeight = smoothing;
}

bool SpeechDetector::isSpeech(float probability, bool gate_open)
{
  if (gate_open)
  {
    smoothed = smoothingWeight * smoothed + (1.0f - smoothingWeight) * probability;
    framesAbove = 0;
    return smoothed >= closeThreshold;
  }
  framesAbove = probability >= openThreshold ? framesAbove + 1 : 0;
  if (framesAbove < onsetFrames)
    return false;
  // the average restarts at the opening, so the silence before the onset does not close the gate again
  smoothed = probability;
  return true;
}

SpeechGate::SpeechGate(int frame_samples, int preroll_frames, int hangover_frames) :
  frameSamples(frame_samples), prerollSamples(preroll_frames * frame_samples), hangoverFrames(hangover_frames)
{
  G_ASSERT(hangover_frames > 0);
}

void SpeechGate::forceOpen()
{
  dropPreroll();
  hangoverLeft = 0;
  startPending = true;
}

void SpeechGate::setTiming(int preroll_frames, int hangover_frames)
{
  G_ASSERT_RETURN(hangover_frames > 0, );
  prerollSamples = preroll_frames * frameSamples;
  hangoverFrames = hangover_frames;
  hangoverLeft = eastl::min(hangoverLeft, hangoverFrames);
  trimPreroll();
}

void SpeechGate::dropPreroll()
{
  counts.dropped += preroll.size() / frameSamples;
  preroll.clear();
}

SpeechGate::Counts SpeechGate::takeCounts()
{
  const Counts taken = counts;
  counts = {};
  return taken;
}

bool SpeechGate::process(dag::ConstSpan<int16_t> frame, bool is_speech, eastl::vector<int16_t> &out)
{
  G_ASSERT_RETURN(frame.size() == frameSamples, false);
  const bool isClosed = hangoverLeft <= 0;
  if (isClosed && !is_speech && !startPending)
  {
    pushPreroll(frame);
    return false;
  }

  const int burstStart = out.size();
  if (isClosed)
    flushPreroll(out);
  out.insert(out.end(), frame.begin(), frame.end());
  ++counts.sent;
  if (isClosed)
  {
    fade_in(make_span(out.data() + burstStart, frameSamples));
    hangoverLeft = hangoverFrames;
    startPending = false;
  }

  if (is_speech)
    hangoverLeft = hangoverFrames;
  else if (--hangoverLeft <= 0)
    fade_out(make_span(out.data() + out.size() - frameSamples, frameSamples));
  return true;
}

void SpeechGate::pushPreroll(dag::ConstSpan<int16_t> frame)
{
  preroll.insert(preroll.end(), frame.begin(), frame.end());
  trimPreroll();
}

void SpeechGate::trimPreroll()
{
  if (int(preroll.size()) > prerollSamples)
  {
    counts.dropped += (preroll.size() - prerollSamples) / frameSamples;
    preroll.erase(preroll.begin(), preroll.end() - prerollSamples);
  }
}

void SpeechGate::flushPreroll(eastl::vector<int16_t> &out)
{
  out.insert(out.end(), preroll.begin(), preroll.end());
  counts.sent += preroll.size() / frameSamples;
  preroll.clear();
}

} // namespace voicechat
