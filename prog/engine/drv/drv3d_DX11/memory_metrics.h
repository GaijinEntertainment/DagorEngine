// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <d3d11.h>
#include <dxgi1_4.h>
#include <supp/dag_comPtr.h>

namespace drv3d_dx11
{

class MemoryMetrics
{
public:
  MemoryMetrics(ID3D11Device *device);
  ~MemoryMetrics() = default;
  void registerReportCallback();
  bool isEnabled() const { return enabled; }
  void update();

private:
  ComPtr<IDXGIAdapter3> adapter{};
  DXGI_QUERY_VIDEO_MEMORY_INFO deviceMemoryInUse{};
  DXGI_QUERY_VIDEO_MEMORY_INFO systemMemoryInUse{};
  bool enabled = false;
};

} // namespace drv3d_dx11
