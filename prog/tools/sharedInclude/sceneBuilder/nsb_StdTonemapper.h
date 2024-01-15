// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __GAIJIN_NSB_STDTONEMAPPER_H__
#define __GAIJIN_NSB_STDTONEMAPPER_H__
#pragma once


#include <supp/dag_math.h>
#include <math/dag_color.h>
#include <math/dag_Point3.h>
#include <sceneBuilder/nsb_decl.h>


// forward declarations for external classes
class DataBlock;


namespace StaticSceneBuilder
{
class StdTonemapper
{
public:
  // Params used in GUI.
  Color3 scaleColor, gammaColor, postScaleColor;
  real scaleMul, gammaMul, postScaleMul;

  Color3 hdrScaleColor, hdrAmbientColor, hdrClampColor, hdrGammaColor, hdrGammaRangeColor;
  real hdrScaleMul, hdrAmbientMul, hdrClampMul, hdrGammaMul, hdrGammaRangeMul;

  real maxHdrLightmapValue;


  StdTonemapper();

  // Calculate internal params from GUI params.
  void recalc();

  void save(DataBlock &blk) const;
  void load(const DataBlock &blk);

  __forceinline Color4 mapColor(const Color4 &color) const
  {
    Color4 res;

    Color4 hdrTransformedColor = transformColorHdr(color);
    res = mapHdrtoLdrColor(hdrTransformedColor);
    res[3] = color[3];

    return res;
  }

  __forceinline Color4 unmapColor(const Color4 &color) const
  {
    Color4 res;

    res = untransformColorHdr(unmapHdrtoLdrColor(color));
    res[3] = color[3];

    return res;
  }

  __forceinline Color4 mapHdrtoLdrColor(const Color4 &hdrTransformedColor) const
  {
    Color4 res;

    for (int j = 0; j < 3; ++j)
    {
      real v = powf(hdrTransformedColor[j] * scale[j], invGamma[j]);
      res[j] = v * postScale[j];
    }

    res[3] = 0;

    return res;
  }

  __forceinline Color4 transformColorHdr(const Color4 &hdr_color) const
  {
    Color3 res = Color3(hdr_color.r, hdr_color.g, hdr_color.b);
    res *= hdrScale;

    res.r = powf(res.r, invHdrGamma.r) * processedHdrGammaRange.r;
    res.g = powf(res.g, invHdrGamma.g) * processedHdrGammaRange.g;
    res.b = powf(res.b, invHdrGamma.b) * processedHdrGammaRange.b;

    res += hdrAmbient;

    if (res.r > hdrClamp.r)
      res.r = hdrClamp.r;
    if (res.g > hdrClamp.g)
      res.g = hdrClamp.g;
    if (res.b > hdrClamp.b)
      res.b = hdrClamp.b;

    return Color4(res.r, res.g, res.b, hdr_color.a);
  }

  __forceinline Color4 unmapHdrtoLdrColor(const Color4 &col) const
  {
    Color4 res;

    for (int j = 0; j < 3; ++j)
    {
      real v = col[j] / postScale[j];
      res[j] = powf(v, 1.0 / invGamma[j]) / scale[j];
    }

    res[3] = 0;

    return res;
  }

  __forceinline Color4 untransformColorHdr(const Color4 &hdr_color) const
  {
    Color3 res = Color3(hdr_color.r, hdr_color.g, hdr_color.b);

    res -= hdrAmbient;

    res.r = powf(res.r / processedHdrGammaRange.r, 1.0 / invHdrGamma.r);
    res.g = powf(res.g / processedHdrGammaRange.g, 1.0 / invHdrGamma.g);
    res.b = powf(res.b / processedHdrGammaRange.b, 1.0 / invHdrGamma.b);

    res /= hdrScale;
    return Color4(res.r, res.g, res.b, hdr_color.a);
  }

  __forceinline Color4 mapHdrToRgbm8(const Color4 &hdr_color)
  {
    Color4 res = transformColorHdr(hdr_color);

    res /= maxHdrLightmapValue;

    real v = res.r;
    if (res.g > v)
      v = res.g;
    if (res.b > v)
      v = res.b;

    if (v < 1e-32f)
      return Color4(0.f, 0.f, 0.f, 0.f);

    // transform by gamma curve for better display brightness fitting
    real mul = powf(v, average(invGamma));

    // round multiplier up to discrete 8-bit value
    mul = ceilf(mul * 255) / 255.0f;

    return Color4(res.r / mul, res.g / mul, res.b / mul, mul);
  }

  __forceinline E3DCOLOR mapHdrToRgbmE3dcolor(const Color4 &hdr_color)
  {
    Color4 c = mapHdrToRgbm8(hdr_color);

    int r = real2int(c.r * 255);
    int g = real2int(c.g * 255);
    int b = real2int(c.b * 255);
    int a = real2int(c.a * 255);

    if (r > 255)
      r = 255;
    if (g > 255)
      g = 255;
    if (b > 255)
      b = 255;
    if (a > 255)
      a = 255;

    return E3DCOLOR(r, g, b, a);
  }

  __forceinline Point3 getScale() const { return Point3(scale.r, scale.g, scale.b); }
  __forceinline Point3 getPostScale() const { return Point3(postScale.r, postScale.g, postScale.b); }
  __forceinline Point3 getInvGamma() const { return Point3(invGamma.r, invGamma.g, invGamma.b); }
  __forceinline Point3 getHdrScale() const { return Point3(hdrScale.r, hdrScale.g, hdrScale.b); }
  __forceinline Point3 getHdrAmbient() const { return Point3(hdrAmbient.r, hdrAmbient.g, hdrAmbient.b); }
  __forceinline Point3 getHdrClamp() const { return Point3(hdrClamp.r, hdrClamp.g, hdrClamp.b); }

protected:
  // Params used for calculation.
  Color3 scale, invGamma, postScale;
  Color3 hdrScale, hdrAmbient, hdrClamp, invHdrGamma, processedHdrGammaRange;
};

class Splitter
{
public:
  Splitter() { defaults(); }

  bool optimizeForVertexCache;
  int trianglesPerObjectTargeted;
  float minRadiusToSplit, maxCombinedSize;
  float smallRadThres, maxCombineRadRatio;
  bool combineObjects;
  bool splitObjects;
  bool buildShadowSD;
  void save(DataBlock &blk);
  void load(const DataBlock &blk, bool init_def = true);

  void defaults()
  {
    trianglesPerObjectTargeted = 500;
    minRadiusToSplit = 10.0;
    smallRadThres = 3;
    maxCombineRadRatio = 1.5;
    combineObjects = true;
    splitObjects = true;
    optimizeForVertexCache = true;
    maxCombinedSize = 200;
    buildShadowSD = false;
  }
};
} // namespace StaticSceneBuilder


#endif
