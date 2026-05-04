//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <math/dag_color.h>
#include <math/dag_TMatrix.h>

#include "renderLightsConsts.hlsli"

struct OmniLight
{
  alignas(16) Point4 pos_radius;
  alignas(16) Color4 color_atten;
  alignas(16) Point4 dir__tex_scale;
  alignas(16) Point4 boxR0;
  alignas(16) Point4 boxR1;
  alignas(16) Point4 boxR2;
  alignas(16) Point4 posRelToOrigin_cullRadius;
  alignas(16) Point4 shadowNearFarClippingPlanesPad;
  OmniLight() {}
  OmniLight(const Point3 &p, const Color3 &col, float rad, float att) :
    pos_radius(p.x, p.y, p.z, rad),
    color_atten(col.r, col.g, col.b, att),
    dir__tex_scale(0, 1, 0, 0),
    posRelToOrigin_cullRadius(0, 0, 0, -1),
    shadowNearFarClippingPlanesPad(0, 0, 0, 0)
  {
    setDefaultBox();
  }
  OmniLight(const Point3 &p, const Color3 &col, float rad, float att, const TMatrix &box) :
    pos_radius(p.x, p.y, p.z, rad),
    color_atten(col.r, col.g, col.b, att),
    dir__tex_scale(0, 1, 0, 0),
    posRelToOrigin_cullRadius(0, 0, 0, -1),
    shadowNearFarClippingPlanesPad(0, 0, 0, 0)
  {
    setBox(box);
  }
  OmniLight(const Point3 &p, const Color3 &col, float rad, float att, const TMatrix *box) :
    pos_radius(p.x, p.y, p.z, rad),
    color_atten(col.r, col.g, col.b, att),
    dir__tex_scale(0, 1, 0, 0),
    posRelToOrigin_cullRadius(0, 0, 0, -1),
    shadowNearFarClippingPlanesPad(0, 0, 0, 0)
  {
    if (box != nullptr && lengthSq(box->getcol(0)) > 0)
      setBox(*box);
    else
      setDefaultBox();
  }
  OmniLight(const Point3 &p, const Point3 &dir, const Color3 &col, float rad, float att, int tex, float texture_scale,
    bool tex_rotation) :
    pos_radius(p.x, p.y, p.z, rad),
    color_atten(col.r, col.g, col.b, att),
    dir__tex_scale(dir.x, dir.y, dir.z, 0),
    posRelToOrigin_cullRadius(0, 0, 0, -1),
    shadowNearFarClippingPlanesPad(0, 0, 0, 0)
  {
    setDefaultBox();
    setTexture(tex, texture_scale, tex_rotation);
  }
  OmniLight(const Point3 &p, const Point3 &dir, const Color3 &col, float rad, float att, int tex, float texture_scale,
    bool tex_rotation, const TMatrix &box, Point2 shadow_zn_zfar = Point2(0, 0)) :
    pos_radius(p.x, p.y, p.z, rad),
    color_atten(col.r, col.g, col.b, att),
    dir__tex_scale(dir.x, dir.y, dir.z, 0),
    posRelToOrigin_cullRadius(0, 0, 0, -1),
    shadowNearFarClippingPlanesPad(0, 0, 0, 0)
  {
    setBox(box);
    setTexture(tex, texture_scale, tex_rotation);

    if (shadow_zn_zfar.x > 0)
      shadowNearFarClippingPlanesPad.x = shadow_zn_zfar.x;

    if (shadow_zn_zfar.y > shadowNearFarClippingPlanesPad.x)
      shadowNearFarClippingPlanesPad.y = shadow_zn_zfar.y;
  }
  void setPos(const Point3 &p)
  {
    pos_radius.x = p.x;
    pos_radius.y = p.y;
    pos_radius.z = p.z;
  }
  void setRadius(float rad) { pos_radius.w = rad; }
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
  }
  void setDirection(const Point3 &dir)
  {
    dir__tex_scale.x = dir.x;
    dir__tex_scale.y = dir.y;
    dir__tex_scale.z = dir.z;
  }
  void setTexture(int tex, float scale, bool rotation)
  {
    if (tex < 0)
      dir__tex_scale.w = 0;
    else
    {
      G_ASSERTF(scale >= M_SQRT1_2 && scale < TEX_ID_MULTIPLIER || (tex & IES_TEX_ID_TYPE_BIT_MASK) != IES_TEX_ID_TYPE__IES,
        "Invalid ies scale value: %f", scale);
      dir__tex_scale.w = float(tex) * TEX_ID_MULTIPLIER + scale;
      if (rotation)
        dir__tex_scale.w *= -1;
    }
  }
  void setBox(const TMatrix &box)
  {
    Point3 len = Point3{lengthSq(box.getcol(0)), lengthSq(box.getcol(1)), lengthSq(box.getcol(2))};
    if (min(len.x, min(len.y, len.z)) > 0)
    {
      boxR0 = Point4{box.getcol(0).x / len.x, box.getcol(1).x / len.y, box.getcol(2).x / len.z, box.getcol(3).x};
      boxR1 = Point4{box.getcol(0).y / len.x, box.getcol(1).y / len.y, box.getcol(2).y / len.z, box.getcol(3).y};
      boxR2 = Point4{box.getcol(0).z / len.x, box.getcol(1).z / len.y, box.getcol(2).z / len.z, box.getcol(3).z};
    }
    else
    {
      boxR0 = Point4{0, 0, 0, 0};
      boxR1 = Point4{0, 0, 0, 0};
      boxR2 = Point4{0, 0, 0, 0};
    }
  }
  void setDefaultBox()
  {
    boxR0 = Point4{0, 0, 0, 0};
    boxR1 = Point4{0, 0, 0, 0};
    boxR2 = Point4{0, 0, 0, 0};
  }
  void setBoxAround(const Point3 &p, float size)
  {
    TMatrix box(size);
    box.setcol(3, p);
    setBox(box);
  }
  void setPosRelToOrigin(const Point3 &pos, float cullRadius)
  {
    posRelToOrigin_cullRadius.x = pos.x;
    posRelToOrigin_cullRadius.y = pos.y;
    posRelToOrigin_cullRadius.z = pos.z;
    posRelToOrigin_cullRadius.w = cullRadius;
  }
  static OmniLight create_empty()
  {
    OmniLight l;
    l.setZero();
    return l;
  }
};
