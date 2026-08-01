// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point3.h>

struct E3DCOLOR;

namespace dafx_ex
{
enum EmitterDebugType
{
  NONE,
  BOX,
  SPHERE,
  CYLINDER,
  CONE,
  SPHERESECTOR
};

struct EmitterDebug
{
  EmitterDebugType type;
  union
  {
    struct
    {
      Point3 offset;
      Point3 dims;
    } box;

    struct
    {
      Point3 offset;
      Point3 vec;
      float h1;
      float h2;
      float rad;
    } cone;

    struct
    {
      Point3 offset;
      float radius;
    } sphere;

    struct
    {
      Point3 offset;
      Point3 vec;
      float radius;
      float height;
    } cylinder;

    struct
    {
      Point3 vec;
      float radius;
      float sector;
    } sphereSector;
  };

  EmitterDebug(const EmitterDebug &emitter)
  {
    type = emitter.type;
    switch (type)
    {
      case EmitterDebugType::BOX: box = emitter.box; break;
      case EmitterDebugType::CONE: cone = emitter.cone; break;
      case EmitterDebugType::SPHERE: sphere = emitter.sphere; break;
      case EmitterDebugType::CYLINDER: cylinder = emitter.cylinder; break;
      case EmitterDebugType::SPHERESECTOR: sphereSector = emitter.sphereSector; break;
      default: break;
    }
  }

  EmitterDebug() : type(NONE) {}

  ~EmitterDebug() {}
};

// To be called between begin_draw_cached_debug_lines() and end_draw_cached_debug_lines(). (or their _ex() variants)
void draw_emitter_debug_cached(const EmitterDebug &emitter_debug, E3DCOLOR color);

} // namespace dafx_ex