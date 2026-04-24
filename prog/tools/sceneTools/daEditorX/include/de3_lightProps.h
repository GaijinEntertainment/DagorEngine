//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_e3dColor.h>
#include <math/dag_color.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_mathBase.h>

struct SunLightProps
{
  inline static const E3DCOLOR DEFAULT_COLOR{255, 255, 242, 255};
  inline static constexpr float DEFAULT_AZIMUTH = 0;
  inline static constexpr float DEFAULT_ZENITH = 40 * DEG_TO_RAD;
  inline static constexpr float DEFAULT_ANG_SIZE = 5 * DEG_TO_RAD;
  inline static constexpr float DEFAULT_FIRST_SUN_BRIGTHNESS = 1600;
  inline static constexpr float DEFAULT_SUN_BRIGTHNESS = 1;

  float azimuth, zenith, angSize;
  E3DCOLOR color;
  float brightness;
  bool enabled;

  Color3 ltCol;
  E3DCOLOR ltColUser;
  float ltColMulUser;

  Point3 ltDir;
  float specPower, specStrength;

  static float calcLuminocity(float br, float ang_size) { return br * (1 - ::cosf(ang_size / 2)); }
  float calcLuminocity() const { return calcLuminocity(brightness, angSize); }

  static float calcBrightness(float lum, float ang_size)
  {
    float denom = (1 - ::cosf(ang_size / 2));
    if (float_nonzero(denom))
      return lum / denom;
    return 0;
  }
  float calcBrightness(float lum) const { return calcBrightness(lum, angSize); }
};

struct SkyLightProps
{
  inline static const E3DCOLOR DEFAULT_COLOR{100, 120, 200, 255};
  inline static constexpr float DEFAULT_BRIGHTNESS = 1.5f;

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
