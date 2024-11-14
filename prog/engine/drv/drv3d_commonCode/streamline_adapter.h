// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_dynLib.h>
#include <3d/dag_nvFeatures.h>
#include <drv/3d/dag_consts.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IPoint2.h>

#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/array.h>
#include <EASTL/vector_map.h>

struct ID3D11Device;
struct ID3D12Device;
struct IDXGIFactory;
struct IDXGIAdapter;

class StreamlineAdapter;

namespace sl
{
struct FrameToken;
}

class StreamlineAdapter : public nv::DLSS, public nv::Reflex
{
public:
  enum class RenderAPI
  {
    DX11,
    DX12
  };

  using SupportOverrideMap = eastl::vector_map<uint32_t, nv::SupportState>;

  static eastl::optional<StreamlineAdapter> create(RenderAPI api, SupportOverrideMap support_override = {});

  StreamlineAdapter(StreamlineAdapter &&) = default;
  StreamlineAdapter &operator=(StreamlineAdapter &&) = default;

  ~StreamlineAdapter();

  void *hook(void *);
  template <typename T>
  T *hook(T *o)
  {
    return static_cast<T *>(hook(static_cast<void *>(o)));
  }

  void setD3DDevice(void *device);
  void setCurrentAdapter(IDXGIAdapter *adapter) { currentAdapter = adapter; }

  nv::SupportState isDlssSupported() const override;
  nv::SupportState isDlssGSupported() const override;
  nv::SupportState isReflexSupported() const override;

  void *getInterposerSymbol(const char *name);

  // Reflex
  void startFrame(uint32_t frame_id) override;
  void initializeReflexState() override;
  bool setMarker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type) override;
  bool setReflexOptions(nv::Reflex::ReflexMode mode, unsigned frameLimitUs) override;
  eastl::optional<nv::Reflex::ReflexState> getReflexState() const override;
  bool sleep() override;
  nv::Reflex::ReflexMode getReflexMode() const;
  uint32_t getFrameId() const;

  // DLSS
  void initializeDlssState();
  bool createDlssFeature(int viewportId, IPoint2 output_resolution, void *commandBuffer);
  bool releaseDlssFeature(int viewportId);
  bool evaluateDlss(uint32_t frame_id, int viewportId, const nv::DlssParams<void> &params, void *commandBuffer);
  eastl::optional<OptimalSettings> getOptimalSettings(Mode mode, IPoint2 output_resolution) const override;
  bool setOptions(int viewportId, Mode mode, IPoint2 output_resolution, float sharpness) override;
  nv::DLSS::State getDlssState() const override { return dlssState; }
  dag::Expected<eastl::string, nv::SupportState> getDlssVersion() const override;

  // DLSS-G
  bool createDlssGFeature(int viewportId, void *commandBuffer);
  bool releaseDlssGFeature(int viewportId);
  bool enableDlssG(int viewportId = 0) override;
  bool disableDlssG(int viewportId = 0) override;
  void setDlssGSuppressed(bool supressed) override;
  bool evaluateDlssG(int viewportId, const nv::DlssGParams<void> &params, void *commandBuffer) override;
  bool isFrameGenerationSupported() const override;
  bool isFrameGenerationEnabled() const override { return frameGenerationEnabled; }
  unsigned getActualFramesPresented() const override;

private:
  using InterposerHandleType = eastl::unique_ptr<void, decltype(&::os_dll_close)>;
  StreamlineAdapter(InterposerHandleType interposer, SupportOverrideMap support_override);

  InterposerHandleType interposer;
  SupportOverrideMap supportOverride;
  static constexpr size_t MAX_CONCURRENT_FRAMES = 8;
  eastl::array<sl::FrameToken *, 8> frameTokens = {};
  uint32_t currentFrameId = 0;
  nv::DLSS::State dlssState = nv::DLSS::State::DISABLED;
  bool frameGenerationEnabled = false;
  IDXGIAdapter *currentAdapter = nullptr;
  nv::Reflex::ReflexMode currentReflexMode = nv::Reflex::ReflexMode::Off;
  bool isSuppressed = false;
};