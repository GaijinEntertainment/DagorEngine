// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <EASTL/vector.h>

namespace voicechat
{

class IVoiceSink;

class VoiceFramer
{
public:
  explicit VoiceFramer(IVoiceSink &sink_);

  IVoiceSink *getSink() const { return sink; }
  // timestamp that the next captured sample gets
  int64_t streamTimeUsec(int64_t now_usec) const { return nextFrameUsec ? nextFrameUsec : now_usec; }
  // an empty pcm flushes a due remainder; after a frame of silence the next pcm starts a new timeline
  void push(dag::ConstSpan<int16_t> pcm, int64_t now_usec);
  // frames given to the sink since the previous call
  int takeFramesSent();

private:
  void send(const int16_t *frame);

  IVoiceSink *sink;
  int64_t frameUsec;
  int64_t nextFrameUsec = 0;
  int64_t lastPcmUsec = 0;
  int64_t lastPushUsec = 0;
  eastl::vector<int16_t> remainder;
  unsigned frameSamples;
  unsigned remainderSamples = 0;
  int framesSent = 0;
};

} // namespace voicechat
