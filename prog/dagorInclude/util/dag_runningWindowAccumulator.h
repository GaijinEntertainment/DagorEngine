//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/array.h>
#include <debug/dag_assert.h>

// @NOTE: this is different from dag_maFilter.h:MeanAvgFilter mainly
// because this allows accumulating sample values within epochs, and
// average over a fixed-size window of epochs. One use case for this
// functionality is timing all invocations of a function in a frame,
// and then averaging over a fixed window of frames.

template <class T, size_t window_size>
class RunningSampleAverageAccumulator
{
public:
  static constexpr ssize_t DONT_USE_EPOCHS = -1;

  void addSample(T sample_val, ssize_t epoch = DONT_USE_EPOCHS)
  {
    if (epoch == DONT_USE_EPOCHS)
      epoch = lastEpoch + 1;

    G_ASSERT(lastEpoch <= epoch);

    if (lastEpoch < epoch)
    {
      for (ssize_t i = lastEpoch + 1; i <= epoch; ++i)
        window[i % window_size] = 0;
    }

    window[epoch % window_size] += sample_val;
    lastEpoch = epoch;
  }

  double getLatestAvg() const
  {
    T sum = 0;
    for (const T val : window)
      sum += val;
    return static_cast<double>(sum) / window_size;
  }

  constexpr int getWindowSize() { return window_size; }

private:
  eastl::array<T, window_size> window{};
  ssize_t lastEpoch = -1;
};
