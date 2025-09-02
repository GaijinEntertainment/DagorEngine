// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_globDef.h>
#include <soundSystem/jitterCompensation.h>
#include "compositeBuffer.h"
#include "speex_headers.h"

#define JITTER_BUFFER ((JitterBuffer *)opaqueBuffer)

static constexpr int MAX_PACKET_SIZE = 2048;


JitterCompensationBuffer::~JitterCompensationBuffer() { jitter_buffer_destroy(JITTER_BUFFER); }


void JitterCompensationBuffer::reset()
{
  jitter_buffer_reset(JITTER_BUFFER);
  remainingSamplesBuf.clear();
  lastConsumedSampleIdx = 0;
}


void JitterCompensationBuffer::addFrame(dag::ConstSpan<uint8_t> pcmData, int sampleIdx)
{
  G_ASSERT_RETURN(pcmData.size() < MAX_PACKET_SIZE, );
  G_ASSERT_RETURN(pcmData.size() % bytesPerSample == 0, );

  int samples = pcmData.size() / bytesPerSample;

  if (!opaqueBuffer)
  {
    samplesPerInputFrame = samples;
    opaqueBuffer = jitter_buffer_init(samplesPerInputFrame);
  }
  G_ASSERT_RETURN(samples == samplesPerInputFrame, );

  JitterBufferPacket pkt;
  pkt.data = reinterpret_cast<char *>(const_cast<uint8_t *>(pcmData.data()));
  pkt.len = pcmData.size();
  pkt.sequence = 0;
  pkt.span = pkt.len / bytesPerSample;
  pkt.timestamp = sampleIdx;
  pkt.user_data = 0;
  jitter_buffer_put(JITTER_BUFFER, &pkt);
}


bool JitterCompensationBuffer::consume(dag::Span<uint8_t> dstData, int &startSampleIdx)
{
  G_ASSERT_RETURN(samplesPerInputFrame > 0, false);

  startSampleIdx = lastConsumedSampleIdx + 1;
  int samplesToGet = dstData.size() / bytesPerSample;
  uint8_t *dst = dstData.data();
  int remainingSamples = remainingSamplesBuf.size() / bytesPerSample;
  bool gotAnySamples = false;
  if (remainingSamples)
  {
    gotAnySamples = true;
    int samplesCopied = min(samplesToGet, remainingSamples);
    int bytesCopied = samplesCopied * bytesPerSample;
    memcpy(dst, remainingSamplesBuf.data(), bytesCopied);
    dst += bytesCopied;
    samplesToGet -= samplesCopied;
    if (samplesCopied == remainingSamples)
    {
      remainingSamplesBuf.clear();
    }
    else
    {
      remainingSamples -= samplesCopied;
      int remainingBytes = remainingSamples * bytesPerSample;
      memmove(remainingSamplesBuf.data(), remainingSamplesBuf.data() + bytesCopied, remainingBytes);
      remainingSamplesBuf.resize(remainingBytes);
    }
    lastConsumedSampleIdx += samplesCopied;
  }

  if (samplesToGet > 0)
  {
    if (consumeFromSpeexBuffer(samplesToGet, dst))
    {
      gotAnySamples = true;
    }
    else if (gotAnySamples)
    {
      memset(dst, 0, samplesToGet * bytesPerSample);
    }

    lastConsumedSampleIdx += samplesToGet;
  }

  return gotAnySamples;
}


bool JitterCompensationBuffer::consumeFromSpeexBuffer(int samplesToGet, uint8_t *dst)
{
  G_ASSERT_RETURN(remainingSamplesBuf.empty(), false);

  char packetData[MAX_PACKET_SIZE];
  JitterBufferPacket pkt;
  int firstSampleIdx = lastConsumedSampleIdx + 1;

  CompositeDstBuffer dstBuf(dag::Span(dst, samplesToGet * bytesPerSample), remainingSamplesBuf, firstSampleIdx, bytesPerSample);

  while (samplesToGet > 0)
  {
    pkt.data = packetData;
    pkt.len = MAX_PACKET_SIZE;
    int result = jitter_buffer_get(JITTER_BUFFER, &pkt, samplesPerInputFrame, nullptr);
    while (result == JITTER_BUFFER_OK)
    {
      dstBuf.assign(pkt.timestamp, dag::Span((uint8_t *)pkt.data, pkt.len));
      pkt.data = packetData;
      pkt.len = MAX_PACKET_SIZE;
      result = jitter_buffer_get_another(JITTER_BUFFER, &pkt);
    }
    jitter_buffer_tick(JITTER_BUFFER);
    samplesToGet -= samplesPerInputFrame;
  }

  if (!dstBuf.finalize())
  {
    G_ASSERT(remainingSamplesBuf.empty());
    return false;
  }

  return true;
}
