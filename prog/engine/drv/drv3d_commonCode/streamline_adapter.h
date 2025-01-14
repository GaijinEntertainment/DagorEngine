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
}

class StreamlineAdapter : public nv::DLSS, public nv::Reflex
{
private:
  struct InitArgs;

public:
  enum class RenderAPI
  {
    DX11,
    DX12
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

#if _TARGET_PC_WIN
  template <typename T>
  static ComPtr<T> hook(ComPtr<T> object)
  {
    ComPtr<T> tmp;
    tmp.Attach(hook(object.Get()));
    return tmp;
  }
#endif

  void setAdapterAndDevice(IDXGIAdapter1 *adapter, ID3D11Device *device);
  void setAdapterAndDevice(IDXGIAdapter1 *adapter, ID3D12Device5 *device);

  void preRecover();
  void recover();

  nv::SupportState isDlssSupported() const override;
  nv::SupportState isDlssGSupported() const override;
  nv::SupportState isReflexSupported() const override;

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

  // Reflex
  void startFrame(uint32_t frame_id) override;
  void initializeReflexState() override;
  bool setMarker(uint32_t frame_id, lowlatency::LatencyMarkerType marker_type) override;
  bool setReflexOptions(nv::Reflex::ReflexMode mode, unsigned frameLimitUs) override;
  eastl::optional<nv::Reflex::ReflexState> getReflexState() const override;
  bool sleep() override;
  nv::Reflex::ReflexMode getReflexMode() const;
  uint32_t getFrameId() const;

private:
  static constexpr size_t MAX_CONCURRENT_FRAMES = 8;

  eastl::unique_ptr<InitArgs> initArgs;
  SupportOverrideMap supportOverride;
#if _TARGET_PC_WIN
  ComPtr<IDXGIAdapter1> adapter;
#endif

  // DLSS/DLSS-G
  nv::DLSS::State dlssState = nv::DLSS::State::DISABLED;
  bool frameGenerationEnabled = false;
  bool isSuppressed = false;

  // Reflex
  nv::Reflex::ReflexMode currentReflexMode = nv::Reflex::ReflexMode::Off;
  uint32_t currentFrameId = 0;
  eastl::array<sl::FrameToken *, MAX_CONCURRENT_FRAMES> frameTokens = {};
};