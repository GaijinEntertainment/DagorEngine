// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <soundSystem/voiceCommunicator.h>
#include <generic/dag_span.h>
#include <EASTL/algorithm.h>
#include <EASTL/vector.h>
#include <string.h>

namespace voicechat
{

// constraint: the owner thread of the communicator calls all of it
class VoiceLoopback final : public IVoiceSource
{
public:
  VoiceLoopback(unsigned sample_rate, unsigned frame_samples, unsigned max_queued_samples) :
    sampleRate(sample_rate), frameSamples(frame_samples), maxQueuedSamples(max_queued_samples)
  {}

  void push(dag::ConstSpan<int16_t> pcm)
  {
    queue.insert(queue.end(), pcm.begin(), pcm.end());
    if (queue.size() > maxQueuedSamples)
      queue.erase(queue.begin(), queue.begin() + (queue.size() - maxQueuedSamples));
  }

  uint32_t getSampleRate() const override { return sampleRate; }
  uint32_t getFrameSamples() const override { return frameSamples; }
  bool makesAudioOnDemand() const override { return false; }
  bool hasAudioFrame() const override { return queue.size() >= frameSamples; }
  void receiveAudioFrame(int16_t *buf, unsigned int buf_size_samples) override
  {
    const unsigned taken = eastl::min<unsigned>(buf_size_samples, queue.size());
    memcpy(buf, queue.data(), taken * sizeof(int16_t));
    memset(buf + taken, 0, (buf_size_samples - taken) * sizeof(int16_t));
    queue.erase(queue.begin(), queue.begin() + taken);
  }

private:
  eastl::vector<int16_t> queue;
  const unsigned sampleRate;
  const unsigned frameSamples;
  const unsigned maxQueuedSamples;
};

} // namespace voicechat
