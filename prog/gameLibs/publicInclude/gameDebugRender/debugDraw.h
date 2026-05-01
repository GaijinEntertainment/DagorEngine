//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace game_dbg
{
struct Primitives2dCache;

#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
void draw_crosshair(Primitives2dCache &prim_cache);
#else
inline void draw_crosshair(Primitives2dCache &) {}
#endif
} // namespace game_dbg
