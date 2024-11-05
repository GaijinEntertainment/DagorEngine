// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resourcePool.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point2.h>
#include <render/world/cameraParams.h>

struct Driver3dPerspective;
class TextureIDPair;
class AntiAliasing
{
public:
  AntiAliasing(const IPoint2 &inputResolution, const IPoint2 &outputResolution);
  virtual ~AntiAliasing() = default;

  // updates projectionMatrix and internal state of the object to prepare for
  // evaluating the next frame
  virtual Point2 update(Driver3dPerspective &perspective);

  struct OptionalInputParams
  {
    Texture *depth;
    Texture *motion;
    Texture *exposure;
    Texture *reactive;
    Driver3dPerspective perspective;
  };

  const IPoint2 &getInputResolution() const { return inputResolution; }
  const IPoint2 &getOutputResolution() const { return outputResolution; }
  virtual void setInputResolution(const IPoint2 &input_res);

  virtual bool needMotionVectors() const { return false; }
  virtual bool needMotionVectorHistory() const { return false; }
  virtual bool needDepthHistory() const { return false; }
  virtual bool supportsReactiveMask() const { return false; }

  virtual float getLodBias() const { return lodBias; }

  bool isUpsampling() const { return inputResolution != outputResolution; }
  Point2 getUpsamplingRatio() const;

  virtual bool isFrameGenerationEnabled() const { return false; }

  // Returns how many frames are considered when applying the AA
  virtual int getTemporalFrameCount() const;

  virtual bool supportsDynamicResolution() const { return false; }

protected:
  static IPoint2 computeInputResolution(const IPoint2 &outputResolution);

  Point2 jitterOffset;

  IPoint2 inputResolution;
  const IPoint2 outputResolution;
  unsigned frameCounter = 0;
  float lodBias;
};