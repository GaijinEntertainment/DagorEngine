// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dafxEmitterDebug.h"

#include <debug/dag_debug3d.h>

namespace dafx_ex
{

void draw_emitter_debug_cached(const EmitterDebug &emitter_debug, E3DCOLOR color)
{
  switch (emitter_debug.type)
  {
    case EmitterDebugType::NONE: break;
    case EmitterDebugType::SPHERE:
    {
      draw_cached_debug_sphere(emitter_debug.sphere.offset, emitter_debug.sphere.radius, color);
    }
    break;
    case EmitterDebugType::BOX:
    {
      BBox3 debugBox;
      debugBox.lim[0] = emitter_debug.box.offset - (emitter_debug.box.dims);
      debugBox.lim[1] = emitter_debug.box.offset + (emitter_debug.box.dims);
      draw_cached_debug_box(debugBox, color);
    }
    break;
    case EmitterDebugType::CYLINDER:
    {
      Point3 position = emitter_debug.cylinder.offset + emitter_debug.cylinder.vec * emitter_debug.cylinder.height * 0.5f;
      Point3 norm;
      if (abs(emitter_debug.cylinder.vec.x) > 0.1)
        norm = emitter_debug.cylinder.vec % Point3(0, 0, 1);
      else
        norm = emitter_debug.cylinder.vec % Point3(1, 0, 0);

      Point3 cross = norm % emitter_debug.cylinder.vec;

      Point3 p1 = position + emitter_debug.cylinder.vec * (emitter_debug.cylinder.height) * 0.5;
      Point3 p2 = position - emitter_debug.cylinder.vec * (emitter_debug.cylinder.height) * 0.5;
      draw_cached_debug_circle(p1, cross, norm, emitter_debug.cylinder.radius, color);
      draw_cached_debug_circle(p2, cross, norm, emitter_debug.cylinder.radius, color);

      const float angleStep = 2 * PI / 4;
      for (int i = 0; i < 4; ++i)
      {
        Quat quaternion = Quat(emitter_debug.cylinder.vec, i * angleStep);
        Point3 newDir = quaternion * norm;
        draw_cached_debug_line(p1 + newDir * emitter_debug.cylinder.radius, p2 + newDir * emitter_debug.cylinder.radius, color);
      }
    }
    break;
    case EmitterDebugType::CONE:
    {
      Point3 position = emitter_debug.cone.offset + emitter_debug.cone.vec * emitter_debug.cone.h1 * 0.5f;
      Point3 norm;
      if (abs(emitter_debug.cone.vec.x) > 0.1)
        norm = normalize(emitter_debug.cone.vec % Point3(0, 0, 1));
      else
        norm = normalize(emitter_debug.cone.vec % Point3(1, 0, 0));

      Point3 cross = normalize(norm % emitter_debug.cone.vec);

      float ro = safediv(emitter_debug.cone.rad, emitter_debug.cone.h2);
      float r3 = emitter_debug.cone.h1 * ro;
      float r2 = r3 + emitter_debug.cone.rad;

      Point3 p1 = position + emitter_debug.cone.vec * (emitter_debug.cone.h1) * 0.5;
      Point3 p2 = position - emitter_debug.cone.vec * (emitter_debug.cone.h1) * 0.5;
      draw_cached_debug_circle(p1, cross, norm, r2, color);
      draw_cached_debug_circle(p2, cross, norm, emitter_debug.cone.rad, color);

      const float angleStep = 2 * PI / 4;
      for (int i = 0; i < 4; ++i)
      {
        Quat quaternion = Quat(emitter_debug.cone.vec, i * angleStep);
        Point3 newDir = quaternion * norm;
        draw_cached_debug_line(p1 + newDir * r2, p2 + newDir * emitter_debug.cone.rad, color);
      }
    }
    break;
    case EmitterDebugType::SPHERESECTOR:
    {
      Point3 vec = normalize(emitter_debug.sphereSector.vec);

      float yAngle = (emitter_debug.sphereSector.sector - 0.5) * PI;
      float s1, c1;
      sincos(yAngle, s1, c1);

      Point3 tv = Point3(c1, s1, c1);

      Point3 bottom = -vec * emitter_debug.sphereSector.radius;
      Point3 top = vec * emitter_debug.sphereSector.radius;

      Point3 norm;
      if (abs(vec.x) > 0.1)
        norm = normalize(vec % Point3(0, 0, 1));
      else
        norm = normalize(vec % Point3(1, 0, 0));

      float lenthFirstCircle = (tv.y + 1.0) * emitter_debug.sphereSector.radius;
      Point3 firstCirclePos = bottom + vec * lenthFirstCircle;

      Point3 cross = normalize(norm % vec);

      float y = length(firstCirclePos - bottom) / (emitter_debug.sphereSector.radius * 2.0);

      float h = 2.0f * emitter_debug.sphereSector.radius * y;
      float radius = sqrtf(max(2.0f * emitter_debug.sphereSector.radius * h - sqr(h), 0.f));
      draw_cached_debug_circle(firstCirclePos, cross, norm, radius, color);

      Point3 p1 = firstCirclePos + cross * radius;
      Point3 p2 = firstCirclePos - cross * radius;
      Point3 p3 = firstCirclePos + norm * radius;
      Point3 p4 = firstCirclePos - norm * radius;

      draw_cached_debug_line(Point3(0, 0, 0), p1, color);
      draw_cached_debug_line(Point3(0, 0, 0), p2, color);
      draw_cached_debug_line(Point3(0, 0, 0), p3, color);
      draw_cached_debug_line(Point3(0, 0, 0), p4, color);

      const int totalSegments = 48;
      const int steps = totalSegments * (1.0f - y) + 1;
      const float angleStep = 2 * PI / 4;
      Point3 posLine2 = Point3(0, 0, 0);
      float radiusLine2 = 0.0f;
      for (int i = 0; i < steps - 1; ++i)
      {
        float h1 = 2.0f * emitter_debug.sphereSector.radius * (i / (float)totalSegments);
        float h2 = 2.0f * emitter_debug.sphereSector.radius * ((i + 1) / (float)totalSegments);
        float radiusLine1 = sqrtf(max(2.0f * emitter_debug.sphereSector.radius * h1 - sqr(h1), 0.f));
        radiusLine2 = sqrtf(max(2.0f * emitter_debug.sphereSector.radius * h2 - sqr(h2), 0.f));
        Point3 posLine1 = top - vec * h1;
        posLine2 = top - vec * h2;
        for (int j = 0; j < 4; ++j)
        {
          Quat quaternion = Quat(vec, j * angleStep);
          Point3 newDir = quaternion * norm;
          draw_cached_debug_line(posLine1 + newDir * radiusLine1, posLine2 + newDir * radiusLine2, color);
        }
      }

      for (int j = 0; j < 4; ++j)
      {
        Quat quaternion = Quat(vec, j * angleStep);
        Point3 newDir = quaternion * norm;
        draw_cached_debug_line(posLine2 + newDir * radiusLine2, firstCirclePos + newDir * radius, color);
      }

      if (emitter_debug.sphereSector.sector < 0.5)
        draw_cached_debug_circle(Point3(0, 0, 0), cross, norm, emitter_debug.sphereSector.radius, color);
    }
    break;
  }
}

} // namespace dafx_ex
