//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_consts.h>
#include <3d/dag_latencyTypes.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IPoint2.h>
#include <util/dag_string.h>

#include <EASTL/optional.h>

class DataBlock;

namespace amd
{

struct FSR
{
  enum class UpscalingMode : int
  {
    Off = 0,
    NativeAA = 1,
    Quality = 2,
    Balanced = 3,
    Performance = 4,
    UltraPerformance = 5,
  };

  struct ContextArgs
  {
    bool enableDebug = false;
    bool invertedDepth = true;
    bool enableHdr = false;
    bool enableFrameGeneration = false;

    uint32_t maxRenderWidth = 0;
    uint32_t maxRenderHeight = 0;
    uint32_t outputWidth = 1;
    uint32_t outputHeight = 1;

    FSR::UpscalingMode mode = FSR::UpscalingMode::Off;
  };

  template <typename TextureType>
  struct UpscalingArgsBase
  {
    UpscalingArgsBase() = default;

    template <typename Other>
    UpscalingArgsBase(const UpscalingArgsBase<Other> &other)
    {
      colorTexture = nullptr;
      depthTexture = nullptr;
      motionVectors = nullptr;
      exposureTexture = nullptr;
      outputTexture = nullptr;
      reactiveTexture = nullptr;
      transparencyAndCompositionTexture = nullptr;
      jitter = other.jitter;
      motionVectorScale = other.motionVectorScale;
      reset = other.reset;
      debugView = other.debugView;
      sharpness = other.sharpness;
      timeElapsed = other.timeElapsed;
      preExposure = other.preExposure;
      inputResolution = other.inputResolution;
      outputResolution = other.outputResolution;
      fovY = other.fovY;
      nearPlane = other.nearPlane;
      farPlane = other.farPlane;
      frameIndex = other.frameIndex;
    }

    TextureType *colorTexture = nullptr;
    TextureType *depthTexture = nullptr;
    TextureType *motionVectors = nullptr;
    TextureType *exposureTexture = nullptr;
    TextureType *outputTexture = nullptr;
    TextureType *reactiveTexture = nullptr;
    TextureType *transparencyAndCompositionTexture = nullptr;
    Point2 jitter = Point2::ZERO;
    Point2 motionVectorScale = Point2::ONE;
    bool reset = false;
    bool debugView = false;
    float sharpness = 0;
    float timeElapsed = 0; // seconds
    float preExposure = 1;
    IPoint2 inputResolution = IPoint2::ONE;
    IPoint2 outputResolution = IPoint2::ONE;
    float fovY = 1;
    float nearPlane = 1;
    float farPlane = 2;
    uint32_t frameIndex = 0;
  };

  using UpscalingArgs = UpscalingArgsBase<Texture>;
  using UpscalingPlatformArgs = UpscalingArgsBase<void>;

  template <typename TextureType>
  struct FrameGenArgsBase
  {
    FrameGenArgsBase() = default;

    template <typename Other>
    FrameGenArgsBase(const FrameGenArgsBase<Other> &other)
    {
      colorTexture = nullptr;
      depthTexture = nullptr;
      motionVectors = nullptr;
      uiTexture = nullptr;
      jitter = other.jitter;
      motionVectorScale = other.motionVectorScale;
      reset = other.reset;
      debugView = other.debugView;
      timeElapsed = other.timeElapsed;
      inputResolution = other.inputResolution;
      outputResolution = other.outputResolution;
      fovY = other.fovY;
      nearPlane = other.nearPlane;
      farPlane = other.farPlane;
      frameIndex = other.frameIndex;
    }

    TextureType *colorTexture = nullptr;
    TextureType *depthTexture = nullptr;
    TextureType *motionVectors = nullptr;
    TextureType *uiTexture = nullptr;
    Point2 jitter = Point2::ZERO;
    Point2 motionVectorScale = Point2::ONE;
    bool reset = false;
    bool debugView = false;
    float timeElapsed = 0; // seconds
    IPoint2 inputResolution = IPoint2::ONE;
    IPoint2 outputResolution = IPoint2::ONE;
    float fovY = 1;
    float nearPlane = 1;
    float farPlane = 2;
    uint32_t frameIndex = 0;
  };

  using FrameGenArgs = FrameGenArgsBase<Texture>;
  using FrameGenPlatformArgs = FrameGenArgsBase<void>;

  static UpscalingMode getUpscalingMode(const DataBlock &video);

  static bool isSupported();

  static int getMaximumNumberOfGeneratedFrames();
};

} // namespace amd