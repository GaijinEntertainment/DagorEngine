//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathUtils.h>

static const float sensorHeightFor35MM = 0.024; // height of 35 mm camera

class DOFProperties
{
public:
  inline bool operator==(const DOFProperties &p) const
  {
    return p.farCoCScale == farCoCScale && p.nearCoCScale == nearCoCScale && p.farCoCBias == farCoCBias &&
           p.nearCoCBias == nearCoCBias;
  }
  inline bool operator!=(const DOFProperties &p) const { return !(*this == p); }

  static inline float get_fNumber(float focalLength, float lenseAperture) { return focalLength / lenseAperture; }
  static inline float get_focalLength(float fNumber, float lenseAperture) { return fNumber * lenseAperture; }

  static inline float get_focalLengthFor35MM(float focalLength, float sensorHeight)
  {
    return focalLength * sensorHeight / sensorHeightFor35MM;
  } // 24mm is 35mm camera height
  bool hasNearDof(float zn, float threshold) const { return calcNearCoc(zn) > threshold; }
  bool hasFarDof(float zf, float threshold) const { return calcFarCoc(zf) > threshold; }
  bool hasCustomFocalLength() const { return customFocalLength; }

  float maxNearDofDist(float threshold) const { return nearDofDist(threshold, 0); }
  float minNearDofDist(float threshold) const { return nearDofDist(threshold, 100000); }
  float maxFarDofDist(float threshold) const { return farDofDist(threshold, 1000000); }
  float minFarDofDist(float threshold) const { return farDofDist(threshold, 0.1) + 1e-6; }

  // set for FoV at 90 degrees (hk = 1)
  // maxBlurAmount is CoC diameter in percent of image!
  inline void setNearDof(float minDist_, float maxDist_, float maxBlurAmount, bool soft = true);
  // set for FoV at 90 degrees (hk = 1)
  // maxBlurAmount is CoC diameter in percent of image!
  inline bool setFarDof(float minDist_, float maxDist_, float maxBlurAmount);

  // filmic camera focused to certain focal Plane. If FocalPlane is negative it is assumed to be at infinity
  // sensorHeight = 0.024 for 35mm camera
  // set for FoV at 90 degrees (hk = 1)
  // float focalLengthFor90FOV = is focalLength of lense divided by tan(theta/2), i.e. angle of lense
  // float focalLengthFor90FOV is SUPPOSED to be sensorHeight*0.5! It is almost always like that in a real world, we keep it as
  // separate param only for clarity of settigns so, focalLength should be derived from sensorHeight (i.e. sensorHeight*0.5)!
  void setFilmicDoF(float focalPlane, float fNumber, float sensorHeight, float focalLength = -1.0f);

  // set for FoV at 90 degrees (hk = 1). Will work almost exactly as setFilmicDoF with focalPlane far enough (1e6)
  // float focalLengthFor90FOV = is focalLength of lense divided by tan(theta/2), i.e. angle of lense
  // float focalLengthFor90FOV = is focalLength of lense divided by tan(theta/2), i.e. angle of lense
  // float focalLengthFor90FOV is SUPPOSED to be sensorHeight*0.5! It is almost always like that in a real world, we keep it as
  // separate param only for clarity of settigns
  void setFilmicInfiniteDoF(float fNumber, float sensorHeight, float focalLengthFor90FOV); // set for FoV at 90 degrees (hk = 1)

  void setNoNearDoF()
  {
    nearCoCScale = nearCoCBias = 0;
    nearLinearMin = nearLinearMax = 0;
  }
  void setNoFarDoF()
  {
    farCoCScale = farCoCBias = 0;
    farLinearMin = farLinearMax = 0;
  }
  inline float calcCoc(float dist) const; // return signed CoC diameter in percents from sensor. negative CoC is near confusion,
                                          // positive - far

  float calcFarCoc(float dist) const { return farCoCScale / max(0.001f, dist) + farCoCBias; }
  float calcNearCoc(float dist) const { return nearCoCScale / max(0.001f, dist) + nearCoCBias; }

  // filmic camera CoC in percent of sensorHeight
  // sensorHeight = 0.024 for 35mm camera
  //"typical" normal lense focalLength (for 35mm camera) is 50mm, i.e. 0.05
  static float calcOpticCoC(float dist, float focalPlane, float focalLength, float fNumber, float sensorHeight)
  {
    dist = max(dist, 0.001f);
    focalPlane = max(focalLength + 0.1f, focalPlane);
    float coeff = (focalLength * focalLength / (fNumber * sensorHeight * (focalPlane - focalLength)));
    return (1 - focalPlane / dist) * coeff;
  }
  void getFarCoCParams(float &scale_for_inv_depth, float &bias_for_inv_depth) const
  {
    scale_for_inv_depth = farCoCScale;
    bias_for_inv_depth = farCoCBias;
  }

  void getNearCoCParams(float &scale_for_inv_depth, float &bias_for_inv_depth) const
  {
    scale_for_inv_depth = nearCoCScale;
    bias_for_inv_depth = nearCoCBias;
  }

  void getNearLinear(float &out_nearLinearMin, float &out_nearLinearMax) const
  {
    out_nearLinearMin = nearLinearMin;
    out_nearLinearMax = nearLinearMax;
  }
  void getFarLinear(float &out_farLinearMin, float &out_farLinearMax) const
  {
    out_farLinearMin = farLinearMin;
    out_farLinearMax = farLinearMax;
  }

protected:
  float farCoCScale = 0, farCoCBias = 0;
  float nearCoCScale = 0, nearCoCBias = 0;
  float farLinearMin = 0, farLinearMax = 0;
  float nearLinearMin = 0, nearLinearMax = 0;
  bool customFocalLength = false;

  float nearDofDist(float threshold, float def) const { return dist_from_coc_scale_bias(nearCoCScale, nearCoCBias, threshold, def); }
  float farDofDist(float threshold, float def) const { return dist_from_coc_scale_bias(farCoCScale, farCoCBias, threshold, def); }
  static float dist_from_coc_scale_bias(float scale, float bias, float threshold, float def)
  {
    float divisor = (threshold - bias);
    float ret = def;
    if (fabsf(divisor) > 1e-6f)
    {
      ret = scale / divisor;
      if (ret < 0)
        return def;
    }
    return ret;
  }
};

inline void DOFProperties::setNearDof(float minDist_, float maxDist_, float maxBlurAmount,
  bool soft) // maxBlurAmount in percent of image!
{
  if (maxBlurAmount < 0.00001)
  {
    setNoNearDoF();
    return;
  }
  float minDist = min(minDist_, maxDist_);
  float maxDist = max(minDist_, maxDist_);
  if (maxDist < 0.001)
  {
    setNoNearDoF();
    return;
  }

  if (soft)
  {
    nearLinearMin = minDist;
    nearLinearMax = maxDist;
  }
  else
  {
    nearLinearMin = nearLinearMax = 0;
  }

  minDist = 1 / max(0.00001f, minDist);
  maxDist = 1 / max(0.00002f, maxDist);
  // min, max are inverted values so it should be max
  nearCoCScale = maxBlurAmount / max(minDist - maxDist, 1e-9f);
  nearCoCBias = -(nearCoCScale * maxDist);
}

inline bool DOFProperties::setFarDof(float minDist_, float maxDist_, float maxBlurAmount)
{
  if (maxBlurAmount < 0.00001)
  {
    setNoFarDoF();
    return false;
  }
  float minDist = min(minDist_, maxDist_);
  float maxDist = max(minDist_, maxDist_);
  if (maxDist < 0.001 || minDist > 1000000)
  {
    setNoFarDoF();
    return false;
  }

  farLinearMin = minDist;
  farLinearMax = maxDist;

  minDist = 1 / max(0.00001f, minDist);
  maxDist = 1 / max(0.00002f, maxDist);
  farCoCScale = maxBlurAmount / min(-1e-9f, maxDist - minDist);
  farCoCBias = -(farCoCScale * minDist);
  return true;
}

inline float DOFProperties::calcCoc(float dist) const
{
  float nearCoC = calcNearCoc(dist);
  float farCoC = calcFarCoc(dist);
  // return max(nearCoC, farCoC);
  if (nearCoC >= farCoC)
    return -nearCoC;
  return farCoC;
}

inline void DOFProperties::setFilmicDoF(float focalPlane, float fNumber, float sensorHeight, float focalLength)
{
  farLinearMin = farLinearMax = 0;
  nearLinearMin = nearLinearMax = 0;

  float focalLengthFor90FOV = focalLength > 0.0f ? focalLength : 0.5 * sensorHeight;
  if (focalPlane <= 0) // -V1051
  {
    setFilmicInfiniteDoF(fNumber, sensorHeight, focalLengthFor90FOV);
    return;
  }
  focalPlane = max(focalLengthFor90FOV + 0.1f, focalPlane);
  float coeff = (focalLengthFor90FOV * focalLengthFor90FOV / (fNumber * sensorHeight * (focalPlane - focalLengthFor90FOV)));

  farCoCScale = -focalPlane * coeff;
  farCoCBias = coeff;
  nearCoCScale = -farCoCScale;
  nearCoCBias = -coeff;
  customFocalLength = focalLength > 0.0f;
}

inline void DOFProperties::setFilmicInfiniteDoF(float fNumber, float sensorHeight, float focalLengthFor90FOV) // set for FoV at 90
                                                                                                              // degrees (hk = 1)
{
  setNoFarDoF();
  // float focalLengthFor90FOV = 0.5*sensorHeight;
  nearCoCScale = (focalLengthFor90FOV * focalLengthFor90FOV) / (fNumber) / sensorHeight;
  nearCoCBias = 0;
}
