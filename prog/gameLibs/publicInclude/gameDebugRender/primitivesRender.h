//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace game_dbg
{
struct PrimitivesCacheStorage;
struct Primitives2dCacheStorage;

#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
void render_primitives(PrimitivesCacheStorage &primitives_cache_storage);
void render_primitives(Primitives2dCacheStorage &primitives_2d_cache_storage);
#else
inline void render_primitives(PrimitivesCacheStorage &) {}
inline void render_primitives(Primitives2dCacheStorage &) {}
#endif
} // namespace game_dbg
