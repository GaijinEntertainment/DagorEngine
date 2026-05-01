//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <dag/dag_vector.h>
#include <EASTL/array.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <math/dag_TMatrix.h>
#include <gameDebugRender/textStyle.h>

namespace game_dbg
{
struct PrimitivesCache
{
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
  struct PrimLine
  {
    Point3 p0 = {0.f, 0.f, 0.f};
    Point3 p1 = {0.f, 0.f, 0.f};
    E3DCOLOR color = E3DCOLOR_MAKE(255, 255, 255, 255);
  };

  struct PrimTriangle
  {
    Point3 p0 = {0.f, 0.f, 0.f};
    Point3 p1 = {0.f, 0.f, 0.f};
    Point3 p2 = {0.f, 0.f, 0.f};
    E3DCOLOR outline = E3DCOLOR_MAKE(255, 255, 255, 255);
    E3DCOLOR fill = 0;
  };

  struct PrimSphere
  {
    Point3 p = {0.f, 0.f, 0.f};
    float radius = 0.f;
    E3DCOLOR color = E3DCOLOR_MAKE(255, 255, 255, 255);
  };

  struct PrimCapsule
  {
    Point3 p0 = {0.f, 0.f, 0.f};
    Point3 p1 = {0.f, 0.f, 0.f};
    float radius = 0.f;
    E3DCOLOR color = E3DCOLOR_MAKE(255, 255, 255, 255);
  };

  struct PrimText
  {
    static const int max_length = 256;

    Point3 p = {0.f, 0.f, 0.f};
    eastl::array<uint8_t, max_length> str;
    TextStyle style;
  };

  struct HolderData
  {
    int64_t id = -1;
    TMatrix tm = TMatrix::IDENT;
    dag::Vector<PrimText> texts;
    dag::Vector<PrimLine> lines;
    dag::Vector<PrimTriangle> triangles;
    dag::Vector<PrimSphere> spheres;
    dag::Vector<PrimCapsule> capsules;
  };
  dag::Vector<HolderData> holders;

  void clear();

  void addLine(int64_t holder_id, const Point3 &p0, const Point3 &p1, const E3DCOLOR &color);
  void addText(int64_t holder_id, const char *text, const Point3 &p, const TextStyle &style);

  /// advanced primitives
  void addSphere(int64_t holder_id, const Point3 &p, float radius, const E3DCOLOR &color);
  void addCapsule(int64_t holder_id, const Point3 &p0, const Point3 &p1, float radius, const E3DCOLOR &color);
  void addCone(int64_t holder_id, const Point3 &p, const Point3 &dir, float angle_cos, float radius, const E3DCOLOR &color);
  void addBox(int64_t holder_id, const Point3 &p, const Point3 &a, const Point3 &b, const Point3 &c, const E3DCOLOR &color);
  void addTriangle(int64_t holder_id, const Point3 &a, const Point3 &b, const Point3 &c, const E3DCOLOR &outline,
    const E3DCOLOR &fill = 0);

  HolderData &getOrAddHolder(int64_t holder_id);
#else
  inline void clear() {}
  inline void addLine(int64_t, const Point3 &, const Point3 &, const E3DCOLOR &) {}
  inline void addText(int64_t, const char *, const Point3 &, const TextStyle &) {}
  inline void addSphere(int64_t, const Point3 &, float, const E3DCOLOR &) {}
  inline void addCapsule(int64_t, const Point3 &, const Point3 &, float, const E3DCOLOR &) {}
  inline void addCone(int64_t, const Point3 &, const Point3 &, float, float, const E3DCOLOR &) {}
  inline void addBox(int64_t, const Point3 &, const Point3 &, const Point3 &, const Point3 &, const E3DCOLOR &) {}
  inline void addTriangle(int64_t, const Point3 &, const Point3 &, const Point3 &, const E3DCOLOR &, const E3DCOLOR &) {}
#endif
};

struct PrimitivesCacheStorage
{
  ska::flat_hash_map<int, PrimitivesCache> data;
  PrimitivesCache &get(int id) { return data[id]; }
};

} // namespace game_dbg
