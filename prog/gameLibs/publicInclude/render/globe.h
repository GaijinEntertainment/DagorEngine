//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_stdint.h>
#include <shaders/dag_shaders.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_Quat.h>
#include <math/dag_Point2.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_textureIDHolder.h>
#include <3d/dag_resPtr.h>
#include <EASTL/string.h>

class DataBlock;
class DaSkies;

class GlobeRenderer
{
public:
  enum class RenderSun : bool
  {
    No,
    Yes
  };
  enum class RenderMoon : bool
  {
    No,
    Yes
  };
  struct Parameters
  {
    struct TextureSliceNames
    {
      eastl::string colorTexName;
      eastl::string normalsTexName;
    };
    eastl::vector<TextureSliceNames> textureSlicesNames;
    eastl::string shadowsTexName;
    eastl::string cloudsTexName;
    float globeSkyLightMul;
    float globeSunLightMul;
    Color4 globeCloudsColor;
    float globeInitialAngle;
    Point3 posOffset;
    bool useGmtTime;
    bool useKmRadiusScale;
    float starsIntensity;
    float sunSize;
    float sunIntensity;
    float moonSize;
    float moonIntensity;
    Color4 globeSaturateColor;
    float globeRotateSpeed;
    bool globeRotateClouds;

    Parameters() = default;
    Parameters(const DataBlock &blk);
  };
  GlobeRenderer() = default;
  GlobeRenderer(const DataBlock &blk);
  GlobeRenderer(Parameters &&parameters);

  double getStarsJulianDay() const { return starsJulianDay; }
  void setStarsJulianDay(double julianDay) { starsJulianDay = julianDay; }

  static Point3 geo_coord_to_cartesian(const Point2 &p);

  void render(DaSkies &skies, RenderSun render_sun, RenderMoon render_moon, const Point3 &view_pos,
    const TextureIDPair &low_res_tex = TextureIDPair());

private:
  void renderEnv(DaSkies &skies, RenderSun render_sun, RenderMoon render_moon, const Driver3dPerspective &persp,
    const Point3 &view_pos);

  Parameters params;
  double starsJulianDay;

  struct TextureSlice
  {
    SharedTex colorTex;
    SharedTex normalsTex;
  };
  eastl::vector<TextureSlice> textureSlices;
  SharedTex shadowsTex;
  SharedTex cloudsTex;
  Point2 texSize;
  int texNumLods;

  PostFxRenderer drawProg;
  PostFxRenderer applyLowProg;
};
