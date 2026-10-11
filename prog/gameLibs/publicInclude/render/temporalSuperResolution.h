//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix4.h>
#include <render/viewDependentResource.h>
#include <render/daFrameGraph/nodeHandle.h>
#include <resourcePool/resourcePool.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_rectInt.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_computeShaders.h>

#include <EASTL/optional.h>
#include <EASTL/fixed_function.h>

class TextureIDPair;

namespace dafg
{
class Registry;
}

namespace render::antialiasing
{

class TemporalSuperResolution
{
public:
  enum class Preset
  {
    High = 1,
    Vr = 2
  };

  static Preset parse_preset(bool vr_mode);

  TemporalSuperResolution(const IPoint2 &input_resolution, const IPoint2 &output_resolution, bool vr_mode = false);
  void releaseShaderResources();
  void setExtraTexFlags(unsigned int flags) { extraTexFlags = flags; }
  dafg::NodeHandle createApplierNode(const char *input_name,
    eastl::fixed_function<32, void(dafg::Registry)> declare_motion_vectors = nullptr);

  struct ApplyTextures
  {
    Texture *inColor = nullptr;
    Texture *outColor = nullptr;
    Texture *historyColor = nullptr;
    Texture *outDepth = nullptr;
    Texture *historyDepth = nullptr;
    Texture *outConfidence = nullptr;
    Texture *historyConfidence = nullptr;
    Texture *reactiveMask = nullptr;
    Texture *vrsMask = nullptr;
    Texture *debug = nullptr;
  };

  void apply(const ApplyTextures &textures, const Point4 &uv_transform, bool reset = false, Point2 jitterPixelOffset = Point2::ZERO,
    IPoint2 input_resolution = IPoint2::ZERO);

  ManagedTexView getDebugRenderTarget();
  unsigned int getOutputTextureFormat() const;

  bool isValid() const { return true; }
  const IPoint2 &getInputResolution() const { return inputResolution; }
  bool isUpsampling() const { return inputResolution != outputResolution; }
  float getLodBias() const { return lodBias; }

private:
  UniqueTex debugTex;

  const IPoint2 inputResolution;
  const IPoint2 outputResolution;

  eastl::unique_ptr<ComputeShaderElement> render_cs;
  eastl::unique_ptr<ComputeShaderElement> render_vr_cs;

  UniqueBuf filterWeightsBuf;

  float lodBias;

  unsigned int extraTexFlags = 0;

  Preset preset;
};

} // namespace render::antialiasing
