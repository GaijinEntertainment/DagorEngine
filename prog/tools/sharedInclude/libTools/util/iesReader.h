//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/vector.h>
#include <math/dag_mathUtils.h>

class ILogWriter;

class IesReader
{
public:
  using Real = double;
  using PixelType = Real;

  struct IesParams
  {
    Real blurRadius = 3 * DEG_TO_RAD;
    Real phiMin = 0;
    Real phiMax = 2 * M_PI;
    Real thetaMin = 0;
    Real thetaMax = M_PI;
    Real edgeFadeout = 0;
  };

  IesReader(IesParams params);
  bool read(const char *filename, ILogWriter &log);

  struct TransformParams
  {
    Real zoom = 1;
    bool rotated = false;
  };

  struct ImageData
  {
    uint32_t width;
    uint32_t height;
    eastl::vector<PixelType> data;
    TransformParams transform;
  };

  ImageData generateSpherical(uint32_t width, uint32_t height, const TransformParams &transform) const;
  ImageData generateOctahedral(uint32_t width, uint32_t height, const TransformParams &transform) const;

  bool isSphericalTransformValid(uint32_t width, uint32_t height, const TransformParams &transform) const;
  bool isOctahedralTransformValid(uint32_t width, uint32_t height, const TransformParams &transform) const;

  // Calculates relative texture area difference between the bounding quad using the used transform
  // and the one using the optimum
  // 1 = entire texture area
  Real getSphericalRelativeWaste(uint32_t width, uint32_t height, const TransformParams &transform) const;
  Real getOctahedralRelativeWaste(uint32_t width, uint32_t height, const TransformParams &transform) const;

  TransformParams getSphericalOptimalTransform(uint32_t width, uint32_t height) const;
  TransformParams getOctahedralOptimalTransform(uint32_t width, uint32_t height, const bool *forced_rotation = nullptr) const;

#ifdef EXPORT_DEF
  static void exportImage(const ImageData &image, const eastl::string &filename);
#endif

  struct OctahedralBounds
  {
    Real rectilinear;
    Real norm1; // L1 norm, max of (abs(x), abs(y))
  } octahedralBounds;

  struct SphericalCoordinates
  {
    Real theta;
    Real phi;
  };

  struct Texcoords
  {
    Real x;
    Real y;
  };

private:
  IesParams params;
  uint32_t lamps = 0;
  Real lumensPerLamp = 0.0f;
  Real multiplier = 0.0f;
  int photometricType = 0;
  int unitsType = 0;
  Real dirX = 0.0f;
  Real dirY = 0.0f;
  Real dirZ = 0.0f;
  Real ballastFactor = 0.0f;
  Real futureUse = 0.0f;
  Real inputWatts = 0.0f;
  eastl::vector<Real> verticalAngles;
  eastl::vector<Real> horizontalAngles;
  eastl::vector<eastl::vector<Real>> candelaValues;
  eastl::vector<eastl::vector<eastl::vector<Real>>> bicubicCoefficients;
  Real maxIntensity = 0.0f;
  Real maxTheta = 0.0f;

  // reads ies data directly at the given dir
  Real sample_raw(SphericalCoordinates coords, uint32_t &baseHInd, uint32_t &baseVInd) const;
  // applies blur and angular restrictions
  Real sample_filtered(SphericalCoordinates coords, Real fadeout, uint32_t &baseHInd, uint32_t &baseVInd) const;

  uint32_t offsetHorizontalIndex(uint32_t index, uint32_t offset) const { return (index + offset) % horizontalAngles.size(); }
  uint32_t nextHorizontalIndex(uint32_t index) const { return offsetHorizontalIndex(index, 1); }
  uint32_t previousHorizontalIndex(uint32_t index) const { return offsetHorizontalIndex(index, horizontalAngles.size() - 1); }
};
