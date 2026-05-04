// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/configuration.h>
#include <driver.h>
#include "generic_tools.h"

#include <debug/dag_log.h>
#include <supp/dag_comPtr.h>


interface DECLSPEC_UUID("9f251514-9d4d-4902-9d60-18988ab7d4b5") DECLSPEC_NOVTABLE IDXGraphicsAnalysis : public IUnknown
{
  STDMETHOD_(void, BeginCapture)() PURE;
  STDMETHOD_(void, EndCapture)() PURE;
};

namespace drv3d_dx12
{
struct Direct3D12Enviroment;
namespace debug
{
namespace gpu_capture
{
struct Issues;

class LegacyPIX : public GenericEventTool, public GenericNamingTool
{
  ComPtr<IDXGraphicsAnalysis> api;
  int framesToCapture = 0;
  bool isCapturing = false;

  static ComPtr<IDXGraphicsAnalysis> try_connect_interface(Direct3D12Enviroment &d3d_env);

  LegacyPIX(ComPtr<IDXGraphicsAnalysis> &&iface) : api{eastl::move(iface)} {}

public:
  void configure();
  void beginCapture(const wchar_t *name);
  void endCapture();
  void onPresent();
  void captureFrames(const wchar_t *, int);
  constexpr bool tryCreateDevice(DXGIAdapter *, UUID, D3D_FEATURE_LEVEL, void **, HLSLVendorExtensions &) { return false; };

  template <typename T>
  static bool connect(const Configuration &config, Direct3D12Enviroment &d3d_env, Issues &, T &target)
  {
    if (!config.enableGPUCapturers)
    {
      return false;
    }

    logdbg("DX12: Looking for legacy PIX capture attachment...");
    auto iface = try_connect_interface(d3d_env);
    if (!iface)
    {
      logdbg("DX12: ...nothing found");
      return false;
    }

    logdbg("DX12: ...found, using legacy PIX interface for frame capturing");
    target = LegacyPIX{eastl::move(iface)};
    return true;
  }
};
} // namespace gpu_capture
} // namespace debug
} // namespace drv3d_dx12
