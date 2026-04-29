// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/configuration.h>
#include <driver.h>
#include "generic_tools.h"

#include <debug/dag_log.h>


struct RENDERDOC_API_1_5_0;

namespace drv3d_dx12
{
struct Direct3D12Enviroment;
namespace debug::gpu_capture
{
struct Issues;

class RenderDoc : public GenericEventTool, public GenericNamingTool
{
  RENDERDOC_API_1_5_0 *api = nullptr;
  int framesToCapture = 0;
  bool isCapturing = false;

  static RENDERDOC_API_1_5_0 *try_connect_interface();

  RenderDoc(RENDERDOC_API_1_5_0 *iface) : api{iface} {}

public:
  void configure();
  void beginCapture(const wchar_t *name);
  void endCapture();
  void onPresent();
  void captureFrames(const wchar_t *, int);
  constexpr bool tryCreateDevice(DXGIAdapter *, UUID, D3D_FEATURE_LEVEL, void **, HLSLVendorExtensions &) { return false; };

  template <typename T>
  static bool connect(const Configuration &config, Direct3D12Enviroment &, Issues &, T &target)
  {
    if (!config.enableGPUCapturers)
    {
      return false;
    }

    logdbg("DX12: Looking for RenderDoc capture attachment...");
    auto iface = try_connect_interface();
    if (!iface)
    {
      logdbg("DX12: ...nothing found");
      return false;
    }

    logdbg("DX12: ...found, using RenderDoc interface for frame capturing");
    target = RenderDoc{iface};
    return true;
  }
};
} // namespace debug::gpu_capture
} // namespace drv3d_dx12
