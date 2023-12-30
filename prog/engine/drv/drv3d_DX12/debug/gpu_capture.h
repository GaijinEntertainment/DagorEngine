#pragma once

#include <EASTL/variant.h>
#include <EASTL/span.h>
#include <supp/dag_comPtr.h>

#include "driver.h"
#include "configuration.h"
#include "winapi_helpers.h"


struct RENDERDOC_API_1_5_0;

interface DECLSPEC_UUID("9f251514-9d4d-4902-9d60-18988ab7d4b5") DECLSPEC_NOVTABLE IDXGraphicsAnalysis : public IUnknown
{
  STDMETHOD_(void, BeginCapture)() PURE;
  STDMETHOD_(void, EndCapture)() PURE;
};

namespace drv3d_dx12
{
struct Direct3D12Enviroment;
}

namespace drv3d_dx12
{
namespace debug
{
namespace gpu_capture
{
class NoTool
{
public:
  void configure() { logdbg("DX12: ...no frame capturing tool is active..."); }
  void beginCapture() {}
  void endCapture() {}
  void onPresent() {}
  void captureFrames(const wchar_t *, int) {}
  void beginEvent(ID3D12GraphicsCommandList *, eastl::span<const char>) {}
  void endEvent(ID3D12GraphicsCommandList *) {}
  void marker(ID3D12GraphicsCommandList *, eastl::span<const char>) {}
};

class RenderDoc
{
  RENDERDOC_API_1_5_0 *api = nullptr;

  static RENDERDOC_API_1_5_0 *try_connect_interface();

  RenderDoc(RENDERDOC_API_1_5_0 *iface) : api{iface} {}

public:
  void configure();
  void beginCapture();
  void endCapture();
  void onPresent();
  void captureFrames(const wchar_t *, int);
  void beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text);
  void endEvent(ID3D12GraphicsCommandList *cmd);
  void marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text);

  template <typename T>
  static bool connect(const Configuration &, Direct3D12Enviroment &, T &target)
  {
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

class LegacyPIX
{
  ComPtr<IDXGraphicsAnalysis> api;
  int framesToCapture = 0;
  bool isCapturing = false;

  static ComPtr<IDXGraphicsAnalysis> try_connect_interface(Direct3D12Enviroment &d3d_env);

  LegacyPIX(ComPtr<IDXGraphicsAnalysis> &&iface) : api{eastl::move(iface)} {}

public:
  void configure();
  void beginCapture();
  void endCapture();
  void onPresent();
  void captureFrames(const wchar_t *, int);
  void beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text);
  void endEvent(ID3D12GraphicsCommandList *cmd);
  void marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text);

  template <typename T>
  static bool connect(const Configuration &, Direct3D12Enviroment &d3d_env, T &target)
  {
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

class PIX
{
  LibPointer runtimeLib;
  LibPointer captureLib;

  static LibPointer try_load_runtime_interface();
  static LibPointer try_connect_capture_interface();
  static LibPointer try_load_capture_interface();

  PIX(LibPointer &&runtime_lib, LibPointer &&capture_lib) : runtimeLib{eastl::move(runtime_lib)}, captureLib{eastl::move(capture_lib)}
  {}

public:
  void configure();
  void beginCapture();
  void endCapture();
  void onPresent();
  void captureFrames(const wchar_t *, int);
  void beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text);
  void endEvent(ID3D12GraphicsCommandList *cmd);
  void marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text);

  template <typename T>
  static bool connect(const Configuration &, Direct3D12Enviroment &, T &target)
  {
    logdbg("DX12: Loading PIX runtime library...");
    auto runtime = try_load_runtime_interface();
    if (!runtime)
    {
      logdbg("DX12: ...failed, using fallback event and marker reporting...");
    }
    logdbg("DX12: Looking for PIX capture attachment...");
    auto capture = try_connect_capture_interface();
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
  static bool load(const Configuration &, Direct3D12Enviroment &, T &target)
  {
    logdbg("DX12: Loading PIX runtime library...");
    auto runtime = try_load_runtime_interface();
    if (!runtime)
    {
      logdbg("DX12: ...failed, using fallback event and marker reporting...");
    }
    logdbg("DX12: Loading PIX capture interface...");
    auto capture = try_load_capture_interface();
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

namespace nvidia
{
class NSight
{
  static bool try_connect_interface();

  NSight() = default;

public:
  void configure();
  void beginCapture();
  void endCapture();
  void onPresent();
  void captureFrames(const wchar_t *, int);
  void beginEvent(ID3D12GraphicsCommandList *, eastl::span<const char>);
  void endEvent(ID3D12GraphicsCommandList *);
  void marker(ID3D12GraphicsCommandList *, eastl::span<const char>);

  template <typename T>
  static bool connect(const Configuration &, Direct3D12Enviroment &, T &target)
  {
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

namespace amd
{
class RadeonGPUProfiler
{
public:
  void configure() {}
  void beginCapture() {}
  void endCapture() {}
  void onPresent() {}
  void captureFrames(const wchar_t *, int) {}
  void beginEvent(ID3D12GraphicsCommandList *, eastl::span<const char>) {}
  void endEvent(ID3D12GraphicsCommandList *) {}
  void marker(ID3D12GraphicsCommandList *, eastl::span<const char>) {}

  template <typename T>
  static bool connect(const Configuration &, Direct3D12Enviroment &, T &)
  {
    return false;
  }
};
} // namespace amd

namespace intel
{
class GraphicsPerformanceAnalyzers
{
public:
  void configure() {}
  void beginCapture() {}
  void endCapture() {}
  void onPresent() {}
  void captureFrames(const wchar_t *, int) {}
  void beginEvent(ID3D12GraphicsCommandList *, eastl::span<const char>) {}
  void endEvent(ID3D12GraphicsCommandList *) {}
  void marker(ID3D12GraphicsCommandList *, eastl::span<const char>) {}

  template <typename T>
  static bool connect(const Configuration &, Direct3D12Enviroment &, T &)
  {
    return false;
  }
};
} // namespace intel
} // namespace gpu_capture

namespace detail
{
// Note that Ts are tried to be constructed in Ts order, so final catch all should be last.
// Note NoT is the default state and represents the state when no of the Ts where constructed.
template <typename NoT, typename... Ts>
class ToolSet
{
public:
  using StorageType = eastl::variant<NoT, Ts...>;
  using DefaultType = NoT;

  template <typename... Is>
  static bool connect(StorageType &store, Is &&...is)
  {
    for (auto connector : {try_to_connect<Ts, Is...>...})
    {
      if (connector(is..., store))
      {
        return true;
      }
    }
    store = DefaultType{};
    return false;
  }

private:
  template <typename T, typename... Is>
  static bool try_to_connect(Is &&...is, StorageType &store)
  {
    return T::connect(is..., store);
  }
};
} // namespace detail
class GpuCapture
{
  using ToolTableType = detail::ToolSet<gpu_capture::NoTool, gpu_capture::nvidia::NSight, gpu_capture::amd::RadeonGPUProfiler,
    gpu_capture::intel::GraphicsPerformanceAnalyzers, gpu_capture::RenderDoc, gpu_capture::PIX, gpu_capture::LegacyPIX>;

  ToolTableType::StorageType tool = ToolTableType::DefaultType{};

public:
  void setup(const Configuration &config, Direct3D12Enviroment &d3d_env)
  {
    // Always try to connect to possible hosting frame debug tool.
    bool aToolIsActive = ToolTableType::connect(tool, config, d3d_env);
#if DAGOR_DBGLEVEL > 0
    // When no tool is active try to load pix capture runtime to allow capturing even without a active frame debug tool.
    if (!aToolIsActive)
    {
      if (config.loadPIXCapturer)
      {
        aToolIsActive = gpu_capture::PIX::load(config, d3d_env, tool);
      }
    }
#else
    G_UNUSED(aToolIsActive);
#endif

    // Some tools may need some one time configuration step we have to do after we are done with the setup.
    eastl::visit([](auto &tool) { tool.configure(); }, tool);
  }
  void teardown() { tool = ToolTableType::DefaultType{}; }
  void beginCapture()
  {
    if (isAnyActive())
    {
      eastl::visit([](auto &tool) { tool.beginCapture(); }, tool);
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
  void beginEvent(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tool) { tool.beginEvent(cmd, text); }, tool);
    }
  }
  void endEvent(ID3D12GraphicsCommandList *cmd)
  {
    if (isAnyActive())
    {
      eastl::visit([=](auto &tool) { tool.endEvent(cmd); }, tool);
    }
  }
  void marker(ID3D12GraphicsCommandList *cmd, eastl::span<const char> text)
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
  bool isAnyPIXActive() const { return isActive<gpu_capture::PIX>() || isActive<gpu_capture::LegacyPIX>(); }
};
} // namespace debug
} // namespace drv3d_dx12