// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/configuration.h>
#include <driver.h>
#include "generic_tools.h"
#include "tools.h"
#include <winapi_helpers.h>

#include <debug/dag_log.h>


namespace drv3d_dx12
{
struct Direct3D12Enviroment;
namespace debug::gpu_capture
{
class PIX : public GenericNamingTool
{
  LibPointer runtimeLib;
  LibPointer captureLib;

  static LibPointer try_load_runtime_interface();
  static LibPointer try_connect_capture_interface(Issues &issues);
  static LibPointer try_load_capture_interface(Issues &issues);

  PIX(LibPointer &&runtime_lib, LibPointer &&capture_lib) : runtimeLib{eastl::move(runtime_lib)}, captureLib{eastl::move(capture_lib)}
  {}

public:
  void configure();
  void beginCapture(const wchar_t *name);
  void endCapture();
  void onPresent();
  void captureFrames(const wchar_t *, int);
  void beginEvent(D3DGraphicsCommandList *cmd, eastl::span<const char> text);
  void endEvent(D3DGraphicsCommandList *cmd);
  void marker(D3DGraphicsCommandList *cmd, eastl::span<const char> text);
  constexpr bool tryCreateDevice(DXGIAdapter *, UUID, D3D_FEATURE_LEVEL, void **, HLSLVendorExtensions &) { return false; };

  template <typename T>
  static bool connect(const Configuration &config, Direct3D12Enviroment &, Issues &issues, T &target)
  {
    if (!config.enableGPUCapturers)
    {
      return false;
    }

    logdbg("DX12: Loading PIX runtime library...");
    auto runtime = try_load_runtime_interface();
    if (!runtime)
    {
      logdbg("DX12: ...failed, using fallback event and marker reporting...");
    }
    logdbg("DX12: Looking for PIX capture attachment...");
    auto capture = try_connect_capture_interface(issues);
    if (!capture)
    {
      logdbg("DX12: ...nothing found");
      return false;
    }

    logdbg("DX12: ...found, using PIX interface for frame capturing");
    target = PIX{eastl::move(runtime), eastl::move(capture)};
    return true;
  }

  // PIX runtime can be loaded to take captures without attached application and allows later attachment of it.
  template <typename T>
  static bool load(const Configuration &, Direct3D12Enviroment &, Issues &issues, T &target)
  {
    logdbg("DX12: Loading PIX runtime library...");
    auto runtime = try_load_runtime_interface();
    if (!runtime)
    {
      logdbg("DX12: ...failed, using fallback event and marker reporting...");
    }
    logdbg("DX12: Loading PIX capture interface...");
    auto capture = try_load_capture_interface(issues);
    if (!capture)
    {
      logdbg("DX12: ...failed");
      return false;
    }

    logdbg("DX12: ...success, using PIX interface for frame capturing");
    target = PIX{eastl::move(runtime), eastl::move(capture)};
    return true;
  }
};
} // namespace debug::gpu_capture
} // namespace drv3d_dx12
