// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "configuration.h"
#include "gpu_capture/tools.h"
#include "gpu_capture/no_tool.h"
#include "gpu_capture/nsight.h"
#if HAS_AGS
#include "gpu_capture/radeon_gpu_profiler.h"
#endif
#include "gpu_capture/graphics_performance_analyzers.h"
#include "gpu_capture/render_doc.h"
#if USE_PIX
#include "gpu_capture/pix.h"
#endif
#include "gpu_capture/legacy_pix.h"


namespace drv3d_dx12::debug
{
class GpuCapture
{
  using ToolTableType = detail::ToolSet<              //
    gpu_capture::NoTool,                              //
    gpu_capture::nvidia::NSight,                      //
#if HAS_AGS                                           //
    gpu_capture::amd::RadeonGPUProfiler,              //
#endif                                                //
    gpu_capture::intel::GraphicsPerformanceAnalyzers, //
    gpu_capture::RenderDoc,                           //
#if USE_PIX                                           //
    gpu_capture::PIX,                                 //
#endif                                                //
    gpu_capture::LegacyPIX                            //
    >;                                                //

  ToolTableType::StorageType tool = ToolTableType::DefaultType{};

public:
  void setup(const Configuration &config, Direct3D12Enviroment &d3d_env, gpu_capture::Issues &issues)
  {
    // Always try to connect to possible hosting frame debug tool.
    [[maybe_unused]]
    bool aToolIsActive = ToolTableType::connect(tool, config, d3d_env, issues);
#if USE_PIX
    // When no tool is active try to load pix capture runtime to allow capturing even without a active frame debug tool.
    if (!aToolIsActive)
    {
      if (config.loadPIXCapturer)
      {
        aToolIsActive = gpu_capture::PIX::load(config, d3d_env, issues, tool);
      }
    }
#endif

    // Some tools may need some one time configuration step we have to do after we are done with the setup.
    eastl::visit([](auto &tool) { tool.configure(); }, tool);
  }
  void teardown() { tool = ToolTableType::DefaultType{}; }
  void beginCapture(const wchar_t *name)
  {
    if (isAnyActive())
    {
      eastl::visit([name](auto &tool) { tool.beginCapture(name); }, tool);
    }
  }
  void endCapture()
  {
    if (isAnyActive())
    {
      eastl::visit([](auto &tool) { tool.endCapture(); }, tool);
    }
  }
  void onPresent()
  {
    if (isAnyActive())
    {
      eastl::visit([](auto &tool) { tool.onPresent(); }, tool);
    }
  }
  void captureFrames(const wchar_t *file_name, int count)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tool) { tool.captureFrames(file_name, count); }, tool);
    }
  }
  void beginEvent(D3DGraphicsCommandList *cmd, eastl::span<const char> text)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tool) { tool.beginEvent(cmd, text); }, tool);
    }
  }
  void endEvent(D3DGraphicsCommandList *cmd)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tool) { tool.endEvent(cmd); }, tool);
    }
  }
  void marker(D3DGraphicsCommandList *cmd, eastl::span<const char> text)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tool) { tool.marker(cmd, text); }, tool);
    }
  }
  template <typename T>
  bool isActive() const
  {
    return eastl::holds_alternative<T>(tool);
  }
  bool isAnyActive() const { return !eastl::holds_alternative<gpu_capture::NoTool>(tool); }
  // PIX has current and legacy mode, to make it easier to check if any is active, have this meta method.
  bool isAnyPIXActive() const
  {
    return
#if USE_PIX
      isActive<gpu_capture::PIX>() ||
#endif
      isActive<gpu_capture::LegacyPIX>();
  }

  bool tryCreateDevice(DXGIAdapter *adapter, UUID uuid, D3D_FEATURE_LEVEL minimum_feature_level, void **ptr,
    HLSLVendorExtensions &extensions)
  {
    return eastl::visit(
      [=, &extensions](auto &tool) { return tool.tryCreateDevice(adapter, uuid, minimum_feature_level, ptr, extensions); }, tool);
  }

  void nameResource(ID3D12Resource *resource, eastl::string_view name)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tool) { tool.nameResource(resource, name); }, tool);
    }
  }

  void nameResource(ID3D12Resource *resource, eastl::wstring_view name)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tool) { tool.nameResource(resource, name); }, tool);
    }
  }

  void nameObject(ID3D12Object *object, eastl::string_view name)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tool) { tool.nameObject(object, name); }, tool);
    }
  }

  void nameObject(ID3D12Object *object, eastl::wstring_view name)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tool) { tool.nameObject(object, name); }, tool);
    }
  }
};
} // namespace drv3d_dx12::debug
