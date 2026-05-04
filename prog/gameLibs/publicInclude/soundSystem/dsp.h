//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_span.h>

class DataBlock;

namespace sndsys
{
namespace dsp
{
void init(const DataBlock &blk);
void close();
void apply();

// Fills num_values floats with normalized frequency bar heights [0..1].
// Uses logarithmic bucketing so bass/mid/treble are distributed perceptually.
// Creates the FMOD FFT DSP on first call; call close() to release it.
void get_spectrum_bars(dag::Span<float> out);
} // namespace dsp
} // namespace sndsys
