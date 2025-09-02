//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_span.h>
#include <generic/dag_tab.h>


class JitterCompensationBuffer final
{
public:
  JitterCompensationBuffer(int bytesPerSample_) : bytesPerSample(bytesPerSample_) {}

  ~JitterCompensationBuffer();

  void addFrame(dag::ConstSpan<uint8_t> pcmData, int sampleIdx);
  bool consume(dag::Span<uint8_t> dstData, int &startSampleIdx);
  int getSamplesPerInputFrame() const { return samplesPerInputFrame; }
  void reset();

private:
  bool consumeFromSpeexBuffer(int samplesToGet, uint8_t *dst);

private:
  Tab<uint8_t> remainingSamplesBuf;
  int lastConsumedSampleIdx = -1;
  int samplesPerInputFrame = 0;
  // can't forward declare JitterBuffer, it's a typedef
  void *opaqueBuffer = nullptr;

  const int bytesPerSample;
};
