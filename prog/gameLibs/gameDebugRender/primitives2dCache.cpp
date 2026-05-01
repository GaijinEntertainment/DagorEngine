// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameDebugRender/primitives2dCache.h>

#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
#include <math/dag_TMatrix.h>
#include <EASTL/fixed_function.h>
#include <generic/dag_carray.h>
#include <osApiWrappers/dag_critSec.h>

static WinCritSec render_data_render_cs;

template <typename T>
static auto &safe_push_back_primitive(T &vec)
{
  if (DAGOR_UNLIKELY(vec.size() == vec.capacity()))
  {
    WinAutoLock lock(render_data_render_cs);
    return vec.push_back();
  }
  return vec.push_back();
}

void game_dbg::Primitives2dCache::addText(const char *text, float x, float y, const TextStyle &style)
{
  PrimText &primText = safe_push_back_primitive(texts);

  primText.x = x;
  primText.y = y;
  primText.style = style;

  strncpy((char *)primText.str.data(), text, PrimText::max_length);
  primText.str[PrimText::max_length - 1] = 0;
}

void game_dbg::Primitives2dCache::clear()
{
  WinAutoLock lock(render_data_render_cs);
  texts.clear();
  lines.clear();
}

void game_dbg::Primitives2dCache::addLine(const Point2 &p0, const Point2 &p1, float thickness, const E3DCOLOR &color)
{
  PrimLine &line = safe_push_back_primitive(lines);

  line.p0 = p0;
  line.p1 = p1;
  line.thickness = thickness;
  line.color = color;
}
#endif
