#pragma once

#include <dxgidebug.h>

#include "gpu_capture.h"
#include "gpu_postmortem.h"

typedef HRESULT(WINAPI *PFN_DXGI_GET_DEBUG_INTERFACE1)(UINT Flags, REFIID riid, void **ppFactory);

namespace drv3d_dx12
{
namespace debug
{
enum class BreadcrumbMode
{
  NONE,
  DRED,
  SOFTWARE,
};

#define DX12_DEBUG_GLOBAL_DEBUG_STATE 1

class GlobalState
{
private:
  GpuCapture gpuCapture;
  GpuPostmortem gpuPostmortem;
  ComPtr<IDXGIDebug> dxgiDebug;
  Configuration config;

public:
  void setup(const DataBlock *settings, Direct3D12Enviroment &d3d_env);
  void teardown();

  GpuCapture &captureTool() { return gpuCapture; }
  GpuPostmortem &postmortemTrace() { return gpuPostmortem; }

  const Configuration &configuration() const { return config; }
};
} // namespace debug
} // namespace drv3d_dx12