// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "memory_metrics.h"

#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_memoryReport.h>
#include <perfMon/dag_statDrv.h>
#include <supp/dag_comPtr.h>
#include "helpers.h"

namespace drv3d_dx11
{

MemoryMetrics::MemoryMetrics(ID3D11Device *device) : enabled(false)
{
  if (DAGOR_LIKELY(device))
  {
    auto activeAdapter = get_active_adapter(device);
    if (activeAdapter && SUCCEEDED(activeAdapter->QueryInterface(COM_ARGS(&adapter))))
      enabled = ::dgs_get_settings()->getBlockByNameEx("video")->getBool("vram_stat", false);
  }
}

void MemoryMetrics::registerReportCallback()
{
  if (!enabled)
    return;

  memoryreport::register_vram_state_callback([this](memoryreport::VRamState &vram_state) {
    G_ASSERT(deviceMemoryInUse.CurrentUsage <= INT64_MAX);
    G_ASSERT(systemMemoryInUse.CurrentUsage <= INT64_MAX);
    vram_state.used_device_local = static_cast<int64_t>(deviceMemoryInUse.CurrentUsage);
    vram_state.used_shared = static_cast<int64_t>(systemMemoryInUse.CurrentUsage);
  });
  logdbg("Registered DX11 vram state callback");
}

void MemoryMetrics::update()
{
  if (!enabled)
    return;

  G_ASSERT_RETURN(adapter, );

  TIME_PROFILE(update_memory_metrics);
  adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &deviceMemoryInUse);
  adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &systemMemoryInUse);
}

} // namespace drv3d_dx11
