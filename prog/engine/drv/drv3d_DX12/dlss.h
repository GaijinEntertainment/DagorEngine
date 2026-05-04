// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_nvFeatures.h>
#include <math/integer/dag_IPoint2.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

struct NVSDK_NGX_Parameter;
struct NVSDK_NGX_Handle;

struct IDXGIAdapter;
struct ID3D12Device;
struct ID3D12GraphicsCommandList;

namespace drv3d_dx12
{

class DLSSSuperResolutionDirect : public nv::DLSS
{
public:
  DLSSSuperResolutionDirect();
  ~DLSSSuperResolutionDirect();

  bool Initialize(ID3D12Device *d3d_device, IDXGIAdapter *dxgi_adapter);
  void Teardown(ID3D12Device *d3d_device);

  bool evaluate(const nv::DlssParams<void> &params, void *command_buffer) override;
  eastl::optional<OptimalSettings> getOptimalSettings(Mode mode, IPoint2 output_resolution) const override;
  bool setOptions(Mode mode, IPoint2 output_resolution, bool use_rr, bool use_legacy_model) override;

  void DeleteFeature() override;

  State getState() override;
  virtual dag::Expected<eastl::string, nv::SupportState> getVersion() const override;

  bool setOptionsBackend(ID3D12GraphicsCommandList *d3d_command_list, Mode mode, IPoint2 output_resolution, bool use_rr,
    bool use_legacy_model);
  eastl::optional<OptimalSettings> getOptimalSettings(Mode mode, IPoint2 output_resolution, NVSDK_NGX_Parameter *params) const;

  bool supportRayReconstruction() override;

  uint64_t getMemoryUsage();

private:
  bool ngxInitialized = false;
  bool isSupported = false;
  bool isRRSupported = false;
  bool useRR = false;
  mutable eastl::vector<eastl::string> requiredDeviceExtensions;
  mutable eastl::vector<eastl::string> requiredInstanceExtensions;

  NVSDK_NGX_Parameter *ngxParams = nullptr;
  NVSDK_NGX_Parameter *ngxParamsForSettings = nullptr;
  NVSDK_NGX_Handle *dlssFeature = nullptr;
};

} // namespace drv3d_dx12