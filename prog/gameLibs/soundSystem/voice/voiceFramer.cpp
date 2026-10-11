// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "voiceFramer.h"
#include <soundSystem/voiceCommunicator.h>
#include <EASTL/algorithm.h>
#include <string.h>

namespace voicechat
{

VoiceFramer::VoiceFramer(IVoiceSink &sink_) :
  sink(&sink_), frameUsec(int64_t(sink_.getFrameSamples()) * 1000000 / sink_.getSampleRate()), frameSamples(sink_.getFrameSamples())
{
  remainder.resize(frameSamples);
}

void VoiceFramer::send(const int16_t *frame)
{
  sink->sendAudioFrame(frame, frameSamples, nextFrameUsec);
  nextFrameUsec += frameUsec;
  ++framesSent;
}

int VoiceFramer::takeFramesSent()
{
  const int frames = framesSent;
  framesSent = 0;
  return frames;
}

void VoiceFramer::push(dag::ConstSpan<int16_t> pcm, int64_t now_usec)
{
  const int64_t prevPushUsec = lastPushUsec;
  lastPushUsec = now_usec;
  if (pcm.empty())
  {
    if (remainderSamples > 0)
    {
      if (nextFrameUsec + frameUsec < now_usec)
      {
        memset(remainder.data() + remainderSamples, 0, (frameSamples - remainderSamples) * sizeof(int16_t));
        send(remainder.data());
        remainderSamples = 0;
        lastPcmUsec = 0;
        nextFrameUsec = 0;
      }
    }
    else if (lastPcmUsec > 0 && prevPushUsec - lastPcmUsec > frameUsec)
    {
      lastPcmUsec = 0;
      nextFrameUsec = 0;
    }
    return;
  }

  if (!nextFrameUsec)
    nextFrameUsec = now_usec;
  lastPcmUsec = now_usec;

  if (remainderSamples > 0)
  {
    const unsigned taken = eastl::min<unsigned>(frameSamples - remainderSamples, pcm.size());
    memcpy(remainder.data() + remainderSamples, pcm.data(), taken * sizeof(int16_t));
    remainderSamples += taken;
    pcm = pcm.subspan(taken);
    if (remainderSamples == frameSamples)
    {
      send(remainder.data());
      remainderSamples = 0;
    }
  }

  for (; pcm.size() >= frameSamples; pcm = pcm.subspan(frameSamples))
    send(pcm.data());

  if (!pcm.empty())
  {
    memcpy(remainder.data(), pcm.data(), pcm.size() * sizeof(int16_t));
    remainderSamples = pcm.size();
  }
}

} // namespace voicechat
