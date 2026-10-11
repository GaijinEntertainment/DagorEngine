// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <EASTL/vector.h>

namespace voicechat
{

// Records what the capture chain gets and gives, so that the voiceProcessing WAV mode can replay a session exactly:
// the raw mic, each far-end frame fed to AEC3, per capture frame the count fed, the record volume and a dropped gate
// preroll, and the output. A dump ends at the seconds asked for, where the capture stream ends, or where its output
// rate leaves the processor's set, whichever is first.
// It holds no denoise or preprocess switch, so a replay needs the switches the session ran with.
// constraint: the capture thread makes the recording calls; write may then run on any one thread
class CaptureDump
{
public:
  // a capture frame of the session: the reference frames fed before it, the record volume it was processed at, and
  // whether the chain dropped its gate preroll before it; one line of _feed.txt
  struct Frame
  {
    int fed = 0;
    float recordVolume = 1.0f;
    bool prerollDropped = false;
    bool operator==(const Frame &other) const
    {
      return fed == other.fed && recordVolume == other.recordVolume && prerollDropped == other.prerollDropped;
    }
  };

  CaptureDump(int capture_rate, int frame_samples, int output_rate, int seconds);

  // a pop of the echo reference: frame when it gave one, an empty span when the capture frame took no more
  void onReferencePop(dag::ConstSpan<int16_t> frame);
  // after one process call, with its whole capture, its output and the record volume it applied
  void onProcessed(dag::ConstSpan<int16_t> capture, dag::ConstSpan<int16_t> output, float record_volume);
  // the chain dropped its gate preroll between two process calls
  void onPrerollDropped() { prerollDropPending = true; }
  bool isFull() const { return mic.size() >= limitSamples; }
  bool isEmpty() const { return mic.empty(); }
  int outputSampleRate() const { return outputRate; }
  // writes <prefix>_mic.wav, _far.wav, _out.wav and _feed.txt, a line "<frames fed> <record volume> <preroll dropped>"
  // per capture frame; false when a file cannot be written
  bool write(const char *prefix) const;

private:
  eastl::vector<int16_t> mic;
  eastl::vector<int16_t> reference;
  eastl::vector<int16_t> out;
  eastl::vector<Frame> frames;
  size_t framesProcessed = 0;
  const int captureRate;
  const int frameSamples;
  const int outputRate;
  const size_t limitSamples;
  // fed so far to the capture frame that has not ended its pops yet
  int pendingFed = 0;
  bool prerollDropPending = false;
};

// seconds that voice.dump_capture asked for, once, then 0; any thread
int take_capture_dump_request();

} // namespace voicechat
