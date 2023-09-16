//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
