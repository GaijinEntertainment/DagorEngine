// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/bitset.h>
#include <daECS/net/sequence.h>
#include <math/dag_mathBase.h>
#include <debug/dag_assert.h>
#include "phys/physConsts.h"

template <typename T, size_t HistorySize, size_t TailSize>
class FixedPlossCalcT
{
public:
  static constexpr size_t HISTORY_SIZE = HistorySize;
  static constexpr size_t TAIL_SIZE = TailSize;

  FixedPlossCalcT() { history.set(); }

  void pushSequence(T seq)
  {
    if (seq == lastSeq)
      return;
    int bitIdx;
    if (net::is_seq_gt(seq, lastSeq)) // seq > lastSeq
    {
      int seqDiff = net::seq_diff(seq, lastSeq);
      G_ASSERT(seqDiff > 0);
      history <<= seqDiff;
      bitIdx = net::seq_diff(T(seq - 1), lastSeq);
      lastSeq = seq;
    }
    else // seq < lastSeq
      bitIdx = net::seq_diff(T(lastSeq - 1), seq);
    if (bitIdx < history.size())
      history.set(bitIdx, true);
  }

  float calcPacketLoss() const // return normalized (unit) value [0..1]
  {
    auto histData = history.data();
    auto prevFirst = histData[0];
    // zero-out tail bits as it's considered not yet known whether it was lost or not
    histData[0] &= ~((typename BitsetType::word_type(1) << TailSize) - 1);
    float ret = 1.f - float(history.count()) / float(HistorySize); // count() should be faster then manual loop over bits
    histData[0] = prevFirst;
    return ret;
  }

  T nextSeq() { return ++lastSeq; }
  void resetSeq() { lastSeq = 0; }

private:
  typedef eastl::bitset<HistorySize + TailSize,
    typename eastl::type_select<TailSize <= 32, EASTL_BITSET_WORD_TYPE_DEFAULT, uint64_t>::type>
    BitsetType;
  mutable BitsetType history;
  T lastSeq = 0;
};

typedef FixedPlossCalcT<net::sequence_t, 9 * PHYS_DEFAULT_TICKRATE, PHYS_DEFAULT_TICKRATE> FixedPlossCalc;
typedef FixedPlossCalcT<net::sequence_t, 9 * PHYS_DEFAULT_TICKRATE, PHYS_DEFAULT_TICKRATE> ControlsPlossCalc;
