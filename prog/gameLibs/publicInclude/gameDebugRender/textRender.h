//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DataBlock;
namespace game_dbg
{
struct Primitives2dCache;
void add_text_block(const DataBlock &text, float x, float y, game_dbg::Primitives2dCache &prim_cache);
} // namespace game_dbg
