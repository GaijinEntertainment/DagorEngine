//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_e3dColor.h>
#include <math/dag_color.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>


struct SunLightProps
{
  float azimuth, zenith, angSize;
  E3DCOLOR color;
  float brightness;
  bool enabled;

  Color3 ltCol;
  E3DCOLOR ltColUser;
  float ltColMulUser;

  Point3 ltDir;
  float specPower, specStrength;

  float calcLuminocity() { return brightness * (1 - ::cosf(angSize / 2)); }
  float calcBrightness(float lum)
  {
    float denom = (1 - ::cosf(angSize / 2));
    if (float_nonzero(denom))
      return lum / denom;
    return 0;
  }
};

struct SkyLightProps
{
  E3DCOLOR color;
  E3DCOLOR earthColor;
  float brightness;
  float earthBrightness;
  bool enabled;

  Color3 ambCol;
  E3DCOLOR ambColUser;
  float ambColMulUser;
};


inline bool change_color(E3DCOLOR &dest, E3DCOLOR c)
{
  bool dif = (c != dest);
  dest = c;
  return dif;
}

inline bool change_float(float &dest, float f)
{
  bool dif = fabs(dest - f) > 1e-6;
  ;
  dest = f;
  return dif;
}

inline bool change_color3(Color3 &dest, const Color3 &f)
{
  bool dif = lengthSq(dest - f) > 1e-12;
  ;
  dest = f;
  return dif;
}

inline bool change_point3(Point3 &dest, const Point3 &f)
{
  bool dif = lengthSq(dest - f) > 1e-12;
  ;
  dest = f;
  return dif;
}
