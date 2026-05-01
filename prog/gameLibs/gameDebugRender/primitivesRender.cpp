// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameDebugRender/primitivesRender.h>

#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN

#include <osApiWrappers/dag_critSec.h>
#include <gui/dag_stdGuiRender.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <math/dag_TMatrix4.h>
#include <debug/dag_debug3d.h>
#include <gameDebugRender/primitivesCache.h>
#include <gameDebugRender/primitives2dCache.h>

static WinCritSec render_data_render_cs;

void game_dbg::render_primitives(Primitives2dCacheStorage &primitives_2d_cache_storage)
{
  WinAutoLock lock(render_data_render_cs);

  StdGuiRender::start_render();

  StdGuiRender::reset_textures();
  StdGuiRender::set_font(0);
  for (auto &primitivesCache : primitives_2d_cache_storage.data)
  {
    for (auto &line : primitivesCache.second.texts)
    {
      StdGuiRender::set_color(line.style.color);
      StdGuiRender::goto_xy(line.x + line.style.offsetX, line.y + line.style.offsetY);
      StdGuiRender::draw_str((const char *)line.str.data());
    }

    for (auto &line : primitivesCache.second.lines)
    {
      StdGuiRender::set_color(line.color);
      StdGuiRender::draw_line(line.p0, line.p1, line.thickness);
    }
  }

  StdGuiRender::end_render();
}

void game_dbg::render_primitives(PrimitivesCacheStorage &primitives_cache_storage)
{
  WinAutoLock lock(render_data_render_cs);

  StdGuiRender::start_render();

  TMatrix4 viewTm;
  d3d::getglobtm(viewTm);
  float zn, zf;
  int viewL, viewT, viewW, viewH;
  d3d::getview(viewL, viewT, viewW, viewH, zn, zf);

  for (auto &primPart : primitives_cache_storage.data)
  {
    for (auto &holder : primPart.second.holders)
    {
      for (auto &textLine : holder.texts)
      {
        Point4 sp = Point4::xyz1(holder.id >= 0 ? holder.tm * textLine.p : textLine.p) * viewTm;
        if (sp.w < VERY_SMALL_NUMBER)
          continue;

        sp /= sp.w;
        if (sp.z > 1.f || fabs(sp.x) >= 1.f || fabs(sp.y) >= 1.f || sp.z < 0.f)
          continue;

        float x = viewL + (sp.x + 1.0f) * 0.5f * viewW + textLine.style.offsetX;
        float y = viewT + (-sp.y + 1.0f) * 0.5f * viewH + textLine.style.offsetY;

        StdGuiRender::set_color(textLine.style.color);
        StdGuiRender::goto_xy(x, y);
        StdGuiRender::draw_str((const char *)textLine.str.data());
      }
    }
  }

  StdGuiRender::end_render();

  ::begin_draw_cached_debug_lines(false, false);

  for (auto &primPart : primitives_cache_storage.data)
  {
    for (auto &holder : primPart.second.holders)
    {
      for (auto &line : holder.lines)
      {
        if (holder.id >= 0)
          ::draw_cached_debug_line(holder.tm * line.p0, holder.tm * line.p1, line.color);
        else
          ::draw_cached_debug_line(line.p0, line.p1, line.color);
      }

      for (auto &sph : holder.spheres)
      {
        if (holder.id >= 0)
          ::draw_cached_debug_sphere(holder.tm * sph.p, sph.radius, sph.color);
        else
          ::draw_cached_debug_sphere(sph.p, sph.radius, sph.color);
      }

      for (auto &caps : holder.capsules)
      {
        if (holder.id >= 0)
          ::draw_cached_debug_capsule_w(holder.tm * caps.p0, holder.tm * caps.p1, caps.radius, caps.color);
        else
          ::draw_cached_debug_capsule_w(caps.p0, caps.p1, caps.radius, caps.color);
      }

      for (const auto &trig : holder.triangles)
      {
        Point3 ps[3] = {trig.p0, trig.p1, trig.p2};
        if (holder.id >= 0)
          for (Point3 &p : ps)
            p = holder.tm * p;
        if (trig.fill != 0)
          ::draw_cached_debug_solid_triangle(ps, trig.fill);
        if (trig.outline != 0)
          for (int i = 0; i < 3; i++)
            ::draw_cached_debug_line(ps[i], ps[(i + 1) % 3], trig.outline);
      }
    }
  }

  ::end_draw_cached_debug_lines();
}
#endif
