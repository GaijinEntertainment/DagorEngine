//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_smallTab.h>

namespace loadmask
{
bool loadMaskFromTga(const char *filename, SmallTab<float, TmpmemAlloc> &hmap, int &w, int &h);
bool loadMaskFromRaw32(const char *filename, SmallTab<float, TmpmemAlloc> &hmap, int &w, int &h);
bool loadMaskFromRaw16(const char *filename, SmallTab<float, TmpmemAlloc> &hmap, int &w, int &h);
bool loadMaskFromFile(const char *filename, SmallTab<float, TmpmemAlloc> &hmap, int &w, int &h);
}; // namespace loadmask
