// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameDebugRender/primitivesCache.h>

#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN

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

void game_dbg::PrimitivesCache::addText(int64_t holder_id, const char *text, const Point3 &p, const TextStyle &style)
{
  PrimText &primText = safe_push_back_primitive(getOrAddHolder(holder_id).texts);

  primText.p = p;
  primText.style = style;

  strncpy((char *)primText.str.data(), text, PrimText::max_length);
  primText.str[PrimText::max_length - 1] = 0;
}

void game_dbg::PrimitivesCache::clear()
{
  WinAutoLock lock(render_data_render_cs);
  holders.clear();
}

void game_dbg::PrimitivesCache::addLine(int64_t holder_id, const Point3 &p0, const Point3 &p1, const E3DCOLOR &color)
{
  PrimLine &line = safe_push_back_primitive(getOrAddHolder(holder_id).lines);

  line.p0 = p0;
  line.p1 = p1;
  line.color = color;
}

void game_dbg::PrimitivesCache::addSphere(int64_t holder_id, const Point3 &p, float radius, const E3DCOLOR &color)
{
  PrimSphere &sphere = safe_push_back_primitive(getOrAddHolder(holder_id).spheres);

  sphere.p = p;
  sphere.radius = radius;
  sphere.color = color;
}

void game_dbg::PrimitivesCache::addCapsule(int64_t holder_id, const Point3 &p0, const Point3 &p1, float radius, const E3DCOLOR &color)
{
  PrimCapsule &capsule = safe_push_back_primitive(getOrAddHolder(holder_id).capsules);

  capsule.p0 = p0;
  capsule.p1 = p1;
  capsule.radius = radius;
  capsule.color = color;
}

void game_dbg::PrimitivesCache::addCone(int64_t holder_id, const Point3 &p, const Point3 &dir, float angle_cos, float radius,
  const E3DCOLOR &color)
{
  Quat quat = quat_rotation_arc(Point3(1, 0, 0), dir);
  float x = angle_cos * radius;
  float y = sqrtf(1.f - sqr(angle_cos)) * radius;
  float yz = y * sqrtf(0.5f);

  addLine(holder_id, p, p + quat * Point3(x, y, 0), color);
  addLine(holder_id, p, p + quat * Point3(x, yz, yz), color);
  addLine(holder_id, p, p + quat * Point3(x, 0, y), color);
  addLine(holder_id, p, p + quat * Point3(x, -yz, yz), color);
  addLine(holder_id, p, p + quat * Point3(x, -y, 0), color);
  addLine(holder_id, p, p + quat * Point3(x, -yz, -yz), color);
  addLine(holder_id, p, p + quat * Point3(x, 0, -y), color);
  addLine(holder_id, p, p + quat * Point3(x, yz, -yz), color);
}

void game_dbg::PrimitivesCache::addBox(int64_t holder_id, const Point3 &p, const Point3 &a, const Point3 &b, const Point3 &c,
  const E3DCOLOR &color)
{
  addLine(holder_id, p, p + a, color);
  addLine(holder_id, p, p + b, color);
  addLine(holder_id, p, p + c, color);
  addLine(holder_id, p + a, p + a + b, color);
  addLine(holder_id, p + a, p + a + c, color);
  addLine(holder_id, p + b, p + b + a, color);
  addLine(holder_id, p + b, p + b + c, color);
  addLine(holder_id, p + c, p + c + a, color);
  addLine(holder_id, p + c, p + c + b, color);
  addLine(holder_id, p + a + b, p + a + b + c, color);
  addLine(holder_id, p + b + c, p + a + b + c, color);
  addLine(holder_id, p + a + c, p + a + b + c, color);
}

void game_dbg::PrimitivesCache::addTriangle(int64_t holder_id, const Point3 &a, const Point3 &b, const Point3 &c,
  const E3DCOLOR &outline, const E3DCOLOR &fill)
{
  PrimTriangle &triangle = safe_push_back_primitive(getOrAddHolder(holder_id).triangles);

  triangle.p0 = a;
  triangle.p1 = b;
  triangle.p2 = c;
  triangle.outline = outline;
  triangle.fill = fill;
}

game_dbg::PrimitivesCache::HolderData &game_dbg::PrimitivesCache::getOrAddHolder(int64_t holder_id)
{
  for (HolderData &holder : holders)
    if (holder.id == holder_id)
      return holder;

  HolderData &holder = holders.push_back();
  holder.id = holder_id;
  return holder;
}
#endif
