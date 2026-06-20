// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>

class DataBlock;

namespace sndsys::steam_audio
{
bool init(const DataBlock &) { return false; }
void shutdown() {}
void update_listener(const TMatrix &) {}
} // namespace sndsys::steam_audio
