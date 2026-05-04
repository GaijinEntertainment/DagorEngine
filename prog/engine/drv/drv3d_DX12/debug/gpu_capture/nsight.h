// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/configuration.h>
#include <driver.h>
#include "generic_tools.h"

#include <debug/dag_log.h>


namespace drv3d_dx12
{
struct Direct3D12Enviroment;
namespace debug::gpu_capture
{
struct Issues;
namespace nvidia
{
class NSight : public GenericEventTool, public GenericNamingTool
{
  static bool try_connect_interface();

  NSight() = default;

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

    logdbg("DX12: Looking for Nvidia NSight...");
    if (!try_connect_interface())
    {
      logdbg("DX12: ...nothing found");
      return false;
    }
    logdbg("DX12: ...found, using fallback event and marker reporting...");
    target = NSight{};
    return true;
  }
};
} // namespace nvidia
} // namespace debug::gpu_capture
} // namespace drv3d_dx12
