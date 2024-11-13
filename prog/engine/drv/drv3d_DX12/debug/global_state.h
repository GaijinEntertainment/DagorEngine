// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "gpu_capture.h"
#include "gpu_postmortem.h"
#include <driver.h>

#include <dxgidebug.h>
#include <supp/dag_comPtr.h>


typedef HRESULT(WINAPI *PFN_DXGI_GET_DEBUG_INTERFACE1)(UINT Flags, REFIID riid, void **ppFactory);

class DataBlock;

namespace drv3d_dx12
{
struct Direct3D12Enviroment;
namespace debug
{
enum class BreadcrumbMode
{
  NONE,
  DRED,
  SOFTWARE,
};

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
  void report();

  GpuCapture &captureTool() { return gpuCapture; }
  GpuPostmortem &postmortemTrace() { return gpuPostmortem; }

  const Configuration &configuration() const { return config; }
};
} // namespace debug
} // namespace drv3d_dx12
