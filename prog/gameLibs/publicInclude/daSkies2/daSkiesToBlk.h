//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daSkies2/daSkies.h>

class DataBlock;

namespace skies_utils
{

void load_daSkies(DaSkies &skies, const DataBlock &blk, int seed, const DataBlock &levelblk);
void save_daSkies(const DaSkies &skies, DataBlock &blk, bool save_min, DataBlock &levelblk);

}; // namespace skies_utils
