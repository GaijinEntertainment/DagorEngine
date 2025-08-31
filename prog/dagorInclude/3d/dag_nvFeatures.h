//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_consts.h>
#include <3d/dag_latencyTypes.h>
#include <math/dag_TMatrix4.h>
#include <generic/dag_expected.h>

#include <EASTL/optional.h>
#include <EASTL/string.h>

namespace nv
{

enum class SupportState
{
  Supported,
  OSOutOfDate,
  DriverOutOfDate,
  AdapterNotSupported,
  DisabledHWS,
  NotSupported
};

struct Camera
{
  TMatrix4 projection;
  TMatrix4 projectionInverse;
  TMatrix4 reprojection;
  TMatrix4 reprojectionInverse;
  TMatrix4 worldToView;
  TMatrix4 viewToWorld;

  Point3 position;
  Point3 up;
  Point3 right;
  Point3 forward;

  float nearZ;
  float farZ;
  float fov;
  float aspect;
};

template <typename HandleType = BaseTexture>
struct DlssParams
{
  HandleType *inColor = nullptr;
  HandleType *inDepth = nullptr;
  HandleType *inMotionVectors = nullptr;
  HandleType *inExposure = nullptr;
  HandleType *inAlbedo = nullptr;
  HandleType *inSpecularAlbedo = nullptr;
  HandleType *inNormalRoughness = nullptr;
  HandleType *inHitDist = nullptr;

  float inJitterOffsetX = .0f;
  float inJitterOffsetY = .0f;
  float inMVScaleX = .0f;
  float inMVScaleY = .0f;
  int inColorDepthOffsetX = 0;
  int inColorDepthOffsetY = 0;
  uint32_t frameId = 0;

  Camera camera = {};

  HandleType *outColor = nullptr;

  uint32_t inColorState = UINT_MAX;
  uint32_t inDepthState = UINT_MAX;
  uint32_t inMotionVectorsState = UINT_MAX;
  uint32_t inExposureState = UINT_MAX;
  uint32_t inAlbedoState = UINT_MAX;
  uint32_t inSpecularAlbedoState = UINT_MAX;
  uint32_t inNormalRoughnessState = UINT_MAX;
  uint32_t inHitDistState = UINT_MAX;

  uint32_t outColorState = UINT_MAX;
};

template <typename InHandleType, typename F>
auto convertDlssParams(const DlssParams<InHandleType> &in,
  F &&converter) -> DlssParams<eastl::remove_pointer_t<decltype(converter(nullptr))>>
{
  return {converter(in.inColor), converter(in.inDepth), converter(in.inMotionVectors), converter(in.inExposure),
    converter(in.inAlbedo), converter(in.inSpecularAlbedo), converter(in.inNormalRoughness), converter(in.inHitDist),
    in.inJitterOffsetX, in.inJitterOffsetY, in.inMVScaleX, in.inMVScaleY, in.inColorDepthOffsetX, in.inColorDepthOffsetY, in.frameId,
    in.camera, converter(in.outColor), in.inColorState, in.inDepthState, in.inMotionVectorsState, in.inExposureState, in.inAlbedoState,
    in.inSpecularAlbedoState, in.inNormalRoughnessState, in.inHitDistState, in.outColorState};
}

template <typename HandleType = BaseTexture>
struct DlssGParams
{
  HandleType *inHUDless = nullptr;
  HandleType *inUI = nullptr;
  HandleType *inDepth = nullptr;
  HandleType *inMotionVectors = nullptr;

  float inJitterOffsetX = .0f;
  float inJitterOffsetY = .0f;
  float inMVScaleX = .0f;
  float inMVScaleY = .0f;
  uint32_t frameId = 0;

  Camera camera = {};

  uint32_t inHUDlessState = UINT_MAX;
  uint32_t inUIState = UINT_MAX;
  uint32_t inDepthState = UINT_MAX;
  uint32_t inMotionVectorsState = UINT_MAX;
};

template <typename InHandleType, typename F>
auto convertDlssGParams(const DlssGParams<InHandleType> &in,
  F &&converter) -> DlssGParams<eastl::remove_pointer_t<decltype(converter(nullptr))>>
{
  return {converter(in.inHUDless), converter(in.inUI), converter(in.inDepth), converter(in.inMotionVectors), in.inJitterOffsetX,
    in.inJitterOffsetY, in.inMVScaleX, in.inMVScaleY, in.frameId, in.camera, in.inHUDlessState, in.inUIState, in.inDepthState,
    in.inMotionVectorsState};
}

struct DLSS
{
  enum class State : int
  {
    NOT_IMPLEMENTED = 0,
    NOT_CHECKED,
    NGX_INIT_ERROR_NO_APP_ID,
    NGX_INIT_ERROR_UNKNOWN,
    NOT_SUPPORTED_OUTDATED_VGA_DRIVER,
    NOT_SUPPORTED_INCOMPATIBLE_HARDWARE,
    NOT_SUPPORTED_32BIT,
    DISABLED,
    SUPPORTED,
    READY
  };

  enum class Mode : int
  {
    Off = -1,
    MaxPerformance = 0,
    Balanced = 1,
    MaxQuality = 2,
    UltraPerformance = 3,
    UltraQuality = 4,
    DLAA = 5
  };

  struct OptimalSettings
  {
    unsigned renderWidth;
    unsigned renderHeight;
    bool rayReconstruction;
  };

  virtual ~DLSS() = default;

  virtual bool evaluate(const nv::DlssParams<void> &params, void *command_buffer) = 0;
  virtual eastl::optional<OptimalSettings> getOptimalSettings(Mode mode, IPoint2 output_resolution) const = 0;
  virtual bool setOptions(Mode mode, IPoint2 output_resolution) = 0;

  // Only to be used with direct DLSS integration
  virtual State getState() { return State::NOT_IMPLEMENTED; }
  virtual dag::Expected<eastl::string, nv::SupportState> getVersion() const { return dag::Unexpected(nv::SupportState::NotSupported); }

  virtual void DeleteFeature() {}

  bool isRR = false;
};

struct DLSSFrameGeneration
{
  virtual void setEnabled(int frames_to_generate) = 0;
  virtual bool isEnabled() const = 0;
  virtual unsigned getActualFramesPresented() const = 0;
  virtual void setSuppressed(bool suppressed) = 0;
};

struct Reflex
{
  struct ReflexStats
  {
    uint64_t frameID;
    uint64_t inputSampleTime;
    uint64_t simStartTime;
    uint64_t simEndTime;
    uint64_t renderSubmitStartTime;
    uint64_t renderSubmitEndTime;
    uint64_t presentStartTime;
    uint64_t presentEndTime;
    uint64_t driverStartTime;
    uint64_t driverEndTime;
    uint64_t osRenderQueueStartTime;
    uint64_t osRenderQueueEndTime;
    uint64_t gpuRenderStartTime;
    uint64_t gpuRenderEndTime;
    uint32_t gpuActiveRenderTimeUs;
    uint32_t gpuFrameTimeUs;
  };

  struct ReflexState
  {
    bool lowLatencyAvailable;
    bool latencyReportAvailable;
    bool flashIndicatorDriverControlled;
    static constexpr size_t FRAME_COUNT = 64;
    ReflexStats stats[FRAME_COUNT];
  };

  enum class ReflexMode
  {
    Off,
    On,
    OnPlusBoost
  };

  virtual void startFrame(uint32_t frame_id) = 0;
  virtual bool setMarker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type) = 0;
  virtual bool setOptions(ReflexMode mode, unsigned frameLimitUs) = 0;
  virtual eastl::optional<ReflexState> getState() const = 0;
  virtual bool sleep(uint32_t frame_id) = 0;
  virtual ReflexMode getCurrentMode() const = 0;
};

struct Streamline
{
  virtual nv::SupportState isDlssSupported() const = 0;
  virtual nv::SupportState isDlssGSupported() const = 0;
  virtual nv::SupportState isDlssRRSupported() const = 0;
  virtual nv::SupportState isReflexSupported() const = 0;

  virtual dag::Expected<eastl::string, nv::SupportState> getDlssVersion() const = 0;

  virtual Reflex *createReflexFeature() = 0;
  virtual void releaseReflexFeature() = 0;

  virtual DLSS *getDlssFeature(int viewport_id) = 0;
  virtual DLSSFrameGeneration *getDlssGFeature(int viewport_id) = 0;
  virtual Reflex *getReflexFeature() = 0;

  virtual int getMaximumNumberOfGeneratedFrames() const = 0;
  virtual bool isDlssModeAvailableAtResolution(nv::DLSS::Mode mode, const IPoint2 &resolution) const = 0;

  // for compatibility
  DLSS::State getDlssState()
  {
    switch (isDlssSupported())
    {
      case nv::SupportState::DisabledHWS: // not reported by dlss
      default:
      case nv::SupportState::NotSupported: return DLSS::State::NGX_INIT_ERROR_UNKNOWN; break;
      case nv::SupportState::AdapterNotSupported: return DLSS::State::NOT_SUPPORTED_INCOMPATIBLE_HARDWARE; break;
      case nv::SupportState::DriverOutOfDate: return DLSS::State::NOT_SUPPORTED_OUTDATED_VGA_DRIVER; break;
      case nv::SupportState::OSOutOfDate: return DLSS::State::NOT_SUPPORTED_32BIT; break;
      case nv::SupportState::Supported: return getDlssFeature(0) ? DLSS::State::READY : DLSS::State::SUPPORTED;
    }
  }
};

} // namespace nv