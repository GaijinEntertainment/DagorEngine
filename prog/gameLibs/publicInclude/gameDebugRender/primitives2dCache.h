//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>
#include <dag/dag_vector.h>
#include <EASTL/array.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <gameDebugRender/textStyle.h>

namespace game_dbg
{
struct Primitives2dCache
{
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
  struct PrimLine
  {
    Point2 p0 = {0.f, 0.f};
    Point2 p1 = {0.f, 0.f};
    float thickness = 0;
    E3DCOLOR color = E3DCOLOR_MAKE(255, 255, 255, 255);
  };

  struct PrimText
  {
    static const int max_length = 256;

    eastl::array<uint8_t, max_length> str;
    float x = 0.f;
    float y = 0.f;
    TextStyle style;
  };

  dag::Vector<PrimText> texts;
  dag::Vector<PrimLine> lines;

  void clear();

  void addLine(const Point2 &p0, const Point2 &p1, float thickness, const E3DCOLOR &color);
  void addText(const char *text, float x, float y, const TextStyle &style);
#else
  inline void clear() {}
  inline void addLine(const Point2 &, const Point2 &, float, const E3DCOLOR &) {}
  inline void addText(const char *, float, float, const TextStyle &) {}
#endif
};

struct Primitives2dCacheStorage
{
  ska::flat_hash_map<int, Primitives2dCache> data;
  Primitives2dCache &get(int id) { return data[id]; }
};
} // namespace game_dbg
