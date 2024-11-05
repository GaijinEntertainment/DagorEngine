//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DataBlock;

namespace sndsys
{
namespace dsp
{
void init(const DataBlock &blk);
void close();
void apply();
} // namespace dsp
} // namespace sndsys
