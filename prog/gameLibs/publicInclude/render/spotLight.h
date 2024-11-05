//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_color.h>

#include "renderLightsConsts.hlsli"

struct SpotLight
{
  alignas(16) Point4 pos_radius;
  alignas(16) Color4 color_atten;
  alignas(16) Point4 dir_tanHalfAngle; // .xyz: normalized dir, w: tan(angle/2)
  alignas(16) Point4 texId_scale;
  float culling_radius;
  bool shadows = false;
  bool contactShadows = false;
  bool requiresCullRadiusOptimization = true;
  SpotLight() : culling_radius(-1.0f), texId_scale(-1, 0, 0, 0) {}
  SpotLight(const Point3 &p, const Color3 &col, float rad, float att, const Point3 &dir, float angle, bool contact_shadows,
    bool shadows) :
    pos_radius(p.x, p.y, p.z, rad),
    color_atten(col.r, col.g, col.b, att),
    dir_tanHalfAngle(dir.x, dir.y, dir.z, angle),
    texId_scale(-1, 0, 0, 0),
    culling_radius(-1.0f),
    shadows(shadows),
    contactShadows(contact_shadows)
  {}
  SpotLight(const Point3 &p, const Color3 &col, float rad, float att, const Point3 &dir, float angle, bool contact_shadows,
    bool shadows, int tex, float texture_scale, bool tex_rotation) :
    pos_radius(p.x, p.y, p.z, rad),
    color_atten(col.r, col.g, col.b, att),
    dir_tanHalfAngle(dir.x, dir.y, dir.z, angle),
    culling_radius(-1.0f),
    shadows(shadows),
    contactShadows(contact_shadows)
  {
    setTexture(tex, texture_scale, tex_rotation);
  }
  void setTexture(int tex, float scale, bool rotation) { texId_scale = Point4(tex, rotation ? -scale : scale, 0, 0); }
  void setPos(const Point3 &p)
  {
    pos_radius.x = p.x;
    pos_radius.y = p.y;
    pos_radius.z = p.z;
  }
  void setRadius(float rad) { pos_radius.w = rad; }
  void setCullingRadius(float rad) { culling_radius = rad; }
  void setColor(const Color3 &c)
  {
    color_atten.r = c.r;
    color_atten.g = c.g;
    color_atten.b = c.b;
  }
  void setZero()
  {
    pos_radius = Point4(0, 0, 0, 0);
    color_atten = Color4(0, 0, 0, 0);
    dir_tanHalfAngle = Point4(0, 0, 1, 1);
    texId_scale = Point4(-1, 0, 0, 0);
  }
  static SpotLight create_empty()
  {
    SpotLight l;
    l.setZero();
    return l;
  }
  struct BoundingSphereDescriptor
  {
    float boundSphereRadius;
    float boundingSphereOffset; // from pos, along dir
  };
  static BoundingSphereDescriptor get_bounding_sphere_description(float light_radius, float sin_half_angle, float cos_half_angle)
  {
    constexpr float COS_PI_4 = 0.70710678118654752440084436210485;
    BoundingSphereDescriptor ret;
    if (cos_half_angle > COS_PI_4) // <=> angle/2 < 45 degrees
    {
      // use circumcircle of the spot light cone
      // the light position is on the surface of the bounding sphere
      ret.boundSphereRadius = light_radius / (2.f * cos_half_angle);
      ret.boundingSphereOffset = ret.boundSphereRadius;
    }
    else
    {
      // only consider the spherical sector
      // the light position is inside of the bounding sphere
      ret.boundSphereRadius = sin_half_angle * light_radius;
      ret.boundingSphereOffset = cos_half_angle * light_radius;
    }
    return ret;
  }
  float getCosHalfAngle() const { return 1. / sqrtf(1 + dir_tanHalfAngle.w * dir_tanHalfAngle.w); }
  // cone bounding sphere; cosHalfAngle can be precalculated to speed up this process
  vec4f getBoundingSphere(float cosHalfAngle) const
  {
    // TODO: further vectorize this (incl getCosHalfAngle()/get_bounding_sphere_description())
    float lightRadius = culling_radius < 0 ? pos_radius.w : culling_radius;
    float sinHalfAngle = dir_tanHalfAngle.w * cosHalfAngle;
    BoundingSphereDescriptor desc = get_bounding_sphere_description(lightRadius, sinHalfAngle, cosHalfAngle);
    vec4f center = v_madd(v_ld(&dir_tanHalfAngle.x), v_splats(desc.boundingSphereOffset), v_ld(&pos_radius.x));
    return v_perm_xyzd(center, v_rot_1(v_set_x(desc.boundSphereRadius)));
  }
  // cone bounding sphere
  vec4f getBoundingSphere() const { return getBoundingSphere(getCosHalfAngle()); }
};
