// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_nvFeatures.h>
#include <drv/3d/dag_consts.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IPoint2.h>
#include <osApiWrappers/dag_dynLib.h>
#include <supp/dag_comPtr.h>

#include <EASTL/array.h>
#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector_map.h>

#if _TARGET_PC_WIN
#include <dxgi.h>
#else
struct IDXGIAdapter1;
#endif
struct ID3D11Device;
struct ID3D12Device5;
struct IUnknown;


namespace sl
{
struct FrameToken;
using Feature = uint32_t;
} // namespace sl

struct FrameTracker
{
  sl::FrameToken &getFrameToken(uint32_t frame_id);
  void startFrame(uint32_t frame_id);

  template <typename Params>
  void initConstants(const Params &f, int viewport_id);

private:
  static constexpr size_t MAX_CONCURRENT_FRAMES = 8;
  static constexpr size_t MAX_VIEWPORTS = 2;
  eastl::array<sl::FrameToken *, MAX_CONCURRENT_FRAMES> frameTokens = {};
  eastl::array<eastl::array<bool, MAX_VIEWPORTS>, MAX_CONCURRENT_FRAMES> constantsInitialized = {};
};

class DLSSSuperResolution : public nv::DLSS
{
public:
  DLSSSuperResolution(int viewport_id, void *command_buffer, FrameTracker &frame_tracker);
  ~DLSSSuperResolution();

  bool evaluate(const nv::DlssParams<void> &params, void *command_buffer) override;
  eastl::optional<OptimalSettings> getOptimalSettings(Mode mode, IPoint2 output_resolution) const override;
  bool setOptions(Mode mode, IPoint2 output_resolution) override;

  static bool isModeAvailableAtResolution(nv::DLSS::Mode mode, const IPoint2 &resolution);

private:
  int viewportId;
  FrameTracker &frameTracker;
};

class DLSSRayReconstruction : public nv::DLSS
{
public:
  DLSSRayReconstruction(int viewport_id, void *command_buffer, FrameTracker &frame_tracker);
  ~DLSSRayReconstruction();

  bool evaluate(const nv::DlssParams<void> &params, void *command_buffer) override;
  eastl::optional<OptimalSettings> getOptimalSettings(Mode mode, IPoint2 output_resolution) const override;
  bool setOptions(Mode mode, IPoint2 output_resolution) override;

private:
  int viewportId;
  FrameTracker &frameTracker;
  nv::DLSS::Mode currentMode = nv::DLSS::Mode::DLAA;
  IPoint2 currentOutputResolution = {0, 0};
};

class DLSSFrameGeneration : public nv::DLSSFrameGeneration
{
public:
  DLSSFrameGeneration(int viewport_id, void *command_buffer, FrameTracker &frame_tracker);
  ~DLSSFrameGeneration();

  void setEnabled(int frames_to_generate) override;
  bool isEnabled() const override { return framesToGenerate > 0; }
  unsigned getActualFramesPresented() const override;
  void setSuppressed(bool suppressed) override;

  bool evaluate(const nv::DlssGParams<void> &params, void *commandBuffer);

  static int getMaximumNumberOfGeneratedFrames();

private:
  int viewportId;
  FrameTracker &frameTracker;
  bool suppressed = false;
  int framesToGenerate = 0;
};

class Reflex : public nv::Reflex
{
public:
  Reflex(FrameTracker &frame_tracker);

  void startFrame(uint32_t frame_id) override;
  bool setMarker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type) override;
  bool setOptions(ReflexMode mode, unsigned frame_limit_us) override;
  eastl::optional<nv::Reflex::ReflexState> getState() const override;
  bool sleep(uint32_t frame_id) override;
  ReflexMode getCurrentMode() const override;

private:
  FrameTracker &frameTracker;
  nv::Reflex::ReflexMode currentReflexMode = nv::Reflex::ReflexMode::Off;
};

class StreamlineAdapter : public nv::Streamline
{
private:
  struct InitArgs;

public:
  enum class RenderAPI
  {
    DX11,
    DX12,
    Vulkan
  };

  using InterposerHandleType = eastl::unique_ptr<void, decltype(&::os_dll_close)>;
  using SupportOverrideMap = eastl::vector_map<uint32_t, nv::SupportState>;

  static InterposerHandleType loadInterposer();
  static void *getInterposerSymbol(const InterposerHandleType &interposer, const char *name);
  static bool init(eastl::optional<StreamlineAdapter> &adapter, RenderAPI api, SupportOverrideMap support_override = {});

  StreamlineAdapter(eastl::unique_ptr<InitArgs> &&initArgs, SupportOverrideMap support_override);
  ~StreamlineAdapter();

  static void *hook(IUnknown *object);
  template <typename T>
  static T *hook(T *object)
  {
    return static_cast<T *>(hook(static_cast<IUnknown *>(object)));
  }

  static void *unhook(IUnknown *object);
  template <typename T>
  static T *unhook(T *object)
  {
    return static_cast<T *>(unhook(static_cast<IUnknown *>(object)));
  }

#if _TARGET_PC_WIN
  template <typename T>
  static ComPtr<T> hook(ComPtr<T> object)
  {
    ComPtr<T> tmp;
    tmp.Attach(hook(object.Get()));
    return tmp;
  }

  template <typename T>
  static ComPtr<T> unhook(ComPtr<T> object)
  {
    ComPtr<T> tmp;
    tmp.Attach(unhook(object.Get()));
    return tmp;
  }
#endif

  void setAdapterAndDevice(IDXGIAdapter1 *adapter, ID3D11Device *device);
  void setAdapterAndDevice(IDXGIAdapter1 *adapter, ID3D12Device5 *device);
  void setVulkan();

  void preRecover();
  void recover();

  nv::SupportState isDlssSupported() const override;
  nv::SupportState isDlssGSupported() const override;
  nv::SupportState isDlssRRSupported() const override;
  nv::SupportState isReflexSupported() const override;

  dag::Expected<eastl::string, nv::SupportState> getDlssVersion() const override;

  nv::DLSS *createDlssFeature(int viewport_id, IPoint2 output_resolution, void *command_buffer);
  DLSSFrameGeneration *createDlssGFeature(int viewport_id, void *command_buffer);
  Reflex *createReflexFeature() override;

  nv::DLSS *getDlssFeature(int viewport_id) { return dlssFeatures[viewport_id].get(); }
  nv::DLSSFrameGeneration *getDlssGFeature(int viewport_id)
  {
    return dlssGFeatures[viewport_id] ? &dlssGFeatures[viewport_id].value() : nullptr;
  }
  nv::Reflex *getReflexFeature() { return reflexFeature ? &reflexFeature.value() : nullptr; }

  void releaseDlssFeature(int viewport_id) { dlssFeatures[viewport_id].reset(); }
  void releaseDlssGFeature(int viewport_id) { dlssGFeatures[viewport_id].reset(); }
  void releaseReflexFeature() override { reflexFeature.reset(); }

  int getMaximumNumberOfGeneratedFrames() const override
  {
    return isDlssGSupported() == nv::SupportState::Supported ? DLSSFrameGeneration::getMaximumNumberOfGeneratedFrames() : 0;
  }

  bool isDlssModeAvailableAtResolution(nv::DLSS::Mode mode, const IPoint2 &resolution) const override
  {
    return isDlssSupported() == nv::SupportState::Supported ? DLSSSuperResolution::isModeAvailableAtResolution(mode, resolution)
                                                            : false;
  }

private:
  eastl::unique_ptr<InitArgs> initArgs;
  SupportOverrideMap supportOverride;
#if _TARGET_PC_WIN
  ComPtr<IDXGIAdapter1> adapter;
#endif

  FrameTracker frameTracker;
  static constexpr size_t MAX_VIEWPORTS = 2;
  eastl::array<eastl::unique_ptr<nv::DLSS>, MAX_VIEWPORTS> dlssFeatures = {};
  eastl::array<eastl::optional<DLSSFrameGeneration>, MAX_VIEWPORTS> dlssGFeatures = {};
  eastl::optional<Reflex> reflexFeature;
};