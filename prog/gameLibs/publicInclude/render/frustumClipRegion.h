//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

/****************************************************************************/
/**
 * Derived / Ported from hlsl code included with the Intel
 * 'Deferred Rendering for Current and Future Rendering Pipelines'
 * http://visual-computing.intel-research.net/art/publications/deferred_rendering/
 */
// Original copyright notice:

// Copyright 2010 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
#include <math/dag_Point4.h>
#include <math/dag_mathBase.h>

//--------------------------------------------------------------------------------------
inline void updateClipRegionRoot(float nc, // Tangent plane x/y normal coordinate (view space)
  float lc,                                // Light x/y coordinate (view space)
  float lz,                                // Light z coordinate (view space)
  float lightRadius,
  float cameraScale, // Project scale for coordinate (_11 or _22 for x/y respectively)
  float &clipMin, float &clipMax)
{

  float nz = safediv(lightRadius - nc * lc, lz);
  float pz = safediv(lc * lc + lz * lz - lightRadius * lightRadius, lz - safediv(nz, nc) * lc);

  if (pz > 0.0f)
  {
    float c = safediv(-nz * cameraScale, nc);
    if (nc > 0.0f)
    {
      // Left side boundary
      clipMin = max(clipMin, c);
    }
    else
    { // Right side boundary
      clipMax = min(clipMax, c);
    }
  }
}

inline void updateClipRegion(float lc, // Light x/y coordinate (view space)
  float lz,                            // Light z coordinate (view space)
  float lightRadius,
  float cameraScale, // Project scale for coordinate (_11 or _22 for x/y respectively)
  float &clipMin, float &clipMax)
{
  float rSq = lightRadius * lightRadius;
  float lcSqPluslzSq = lc * lc + lz * lz;
  float d = rSq * lc * lc - lcSqPluslzSq * (rSq - lz * lz);

  if (d >= 0.0f)
  {
    float a = lightRadius * lc;
    float b = sqrtf(d);
    float nx0 = safediv(a + b, lcSqPluslzSq);
    float nx1 = safediv(a - b, lcSqPluslzSq);

    updateClipRegionRoot(nx0, lc, lz, lightRadius, cameraScale, clipMin, clipMax);
    updateClipRegionRoot(nx1, lc, lz, lightRadius, cameraScale, clipMin, clipMax);
  }
}

// Returns bounding box [min.xy, max.xy] in clip [-1, 1] space.
inline Point4 computeClipRegion(const Point4 &lightPosView, float cameraNear, float m11, float m22)
{
  // Early out with empty rectangle if the light is too far behind the view frustum
  Point4 clipRegion(1.0f, 1.0f, -1.0f, -1.0f);
  if (lightPosView.z + lightPosView.w >= cameraNear)
  {
    clipRegion = Point4(-1, -1, 1, 1);

    updateClipRegion(lightPosView.x, lightPosView.z, lightPosView.w, m11, clipRegion.x, clipRegion.z);
    updateClipRegion(lightPosView.y, lightPosView.z, lightPosView.w, m22, clipRegion.y, clipRegion.w);
  }

  return clipRegion;
}

struct FrustumScreenRect
{
  uint8_t min_x, max_x, min_y, max_y;
  int center_y;
  // int16_t center_x, center_y;
};

inline FrustumScreenRect findScreenSpaceBounds(float m11, float m22, const Point4 &pt, float width, float height, float near)
{
  Point4 reg = computeClipRegion(pt, near, m11, m22);

  // swap(reg.x, reg.z);
  // swap(reg.y, reg.w);
  reg = mul(reg, Point4(0.5f, -0.5f, 0.5f, -0.5f)) + Point4(0.5, 0.5, 0.5, 0.5);
  FrustumScreenRect result;
  // result.center_x = int16_t( (reg.x + reg.z) *0.5f* width);
  result.center_y = int((reg.y + reg.w) * 0.5f * height);

  static const Point4 zeros(0, 0, 0, 0), ones(1 - 0.00390625, 1 - 0.00390625, 1 - 0.00390625, 1 - 0.00390625); // 1/-1/256.
  reg = max(min(reg, ones), zeros);

  result.min_x = uint8_t(reg.x * (width));
  result.min_y = uint8_t(reg.w * (height)); // swapped
  result.max_x = uint8_t(reg.z * (width));
  result.max_y = uint8_t(reg.y * (height)); // swapped

  G_ASSERT(result.max_x <= uint32_t(width));
  G_ASSERT(result.max_y <= uint32_t(height));

  return result;
}
