//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_color.h>
#include <math/dag_TMatrix.h>

// Light values computed from the light components, shared by the light ES and tools.
// color is light__color * light__brightness. light__force_max_light_radius must be set only with light__max_radius > 0.

inline float calc_omni_light_radius(const Color3 &color, float light__radius_scale, float light__max_radius,
  bool light__force_max_light_radius)
{
  const float dropOffLuminance = 0.05; // 5% of the original luminance
  const float baseLuminance = average(color);
  // This is the distance where most of the luminance (95%) drops off due to inverse square law.
  // Use the light__radius_scale to adjust the drop-off radius
  float autoDropOffRadius = sqrtf(max(0.0f, baseLuminance / dropOffLuminance)) * light__radius_scale;
  if (light__max_radius > 0)
    autoDropOffRadius = min(light__max_radius, autoDropOffRadius);

  if (light__force_max_light_radius && light__max_radius <= 0)
    light__force_max_light_radius = false;

  return light__force_max_light_radius ? light__max_radius : autoDropOffRadius;
}

inline float calc_spot_light_radius(const Color3 &color, float light__radius_scale, float light__max_radius,
  bool light__force_max_light_radius)
{
  float auto_radius = light__radius_scale * rgbsum(color) * (4.47f / 3); // 4.47 = 1/sqrt(0.05)/ (3 for rgbsum), i.e. 95% of brightness
                                                                         // has already lost due to inverse squares law. If light is in
                                                                         // very dark place, it can be increased by scale
  if (light__max_radius > 0)                                             //-V1051
    auto_radius = min(light__max_radius, auto_radius);

  if (light__force_max_light_radius && light__max_radius <= 0)
    light__force_max_light_radius = false;

  return light__force_max_light_radius ? light__max_radius : auto_radius;
}

// spot_light__cone_angle is the full cone angle in degrees.
// A negative value makes the half tangent the Z column length  of the transform
// (an unscaled transform gives half tangent 1, a 45 degree half angle and a 90 degree cone).
inline float calc_spot_light_half_tan(float spot_light__cone_angle, const TMatrix &transform)
{
  float halfTan = length(transform.getcol(2));

  if (spot_light__cone_angle >= 0)
  {
    float angle = spot_light__cone_angle * PI / 180.0f;
    halfTan = tanf(angle / 2);
  }
  return halfTan;
}

inline Point3 calc_spot_light_direction(const TMatrix &transform)
{
  const float col_length = length(transform.getcol(2));
  constexpr float EPS = 1e-6;
  const Point3 worldUpDir(0.f, 1.f, 0.f);
  return col_length > EPS ? transform.getcol(2) / col_length : worldUpDir;
}

inline float calc_spot_light_illuminating_disc_offset(float spot_light__illuminating_disc_radius, float halfTan)
{
  return safediv(spot_light__illuminating_disc_radius, halfTan);
}

inline Point3 calc_spot_light_apex(const TMatrix &transform, const Point3 &light__offset, const Point3 &direction,
  float illuminatingDiscOffset)
{
  return transform * light__offset - direction * illuminatingDiscOffset;
}
