// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class DataBlock;
class TMatrix;

namespace sndsys::steam_audio
{
bool init(const DataBlock &blk);
void shutdown();
void update_listener(const TMatrix &listener_tm);
} // namespace sndsys::steam_audio
