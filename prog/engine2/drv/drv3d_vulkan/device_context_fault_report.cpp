#include "device.h"
#include "swapchain.h"
#include <osApiWrappers/dag_files.h>
#include <EASTL/sort.h>
#include <EASTL/functional.h>
#include <perfMon/dag_statDrv.h>
#include "buffer.h"
#include "texture.h"
#include "util/fault_report.h"

using namespace drv3d_vulkan;

void dumpDeviceFaultExtData(Device &drvDev, FaultReportDump &dump)
{
  enum
  {
    ID_STATUS,
    ID_COUNTS,
    ID_RECIVED,
    ID_DESC,
    ID_VENDOR_DUMP,
    ID_REPORT_DATA
  };

  if (!drvDev.getDeviceProperties().hasDeviceFaultFeature)
  {
    dump.addTagged(FaultReportDump::GlobalTag::TAG_EXT_FAULT, ID_STATUS, String("ext is not available"));
    return;
  }

#if VK_EXT_device_fault

  VulkanDevice &vkDev = drvDev.getVkDevice();

  VkDeviceFaultCountsEXT counts = {VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT, nullptr, 0, 0, 0};
  if (VULKAN_FAIL(vkDev.vkGetDeviceFaultInfoEXT(vkDev.get(), &counts, nullptr)))
  {
    dump.addTagged(FaultReportDump::GlobalTag::TAG_EXT_FAULT, ID_STATUS, String("element count can't be recived"));
    return;
  }

  // sanity check
  if (!drvDev.getDeviceProperties().hasDeviceFaultVendorInfo)
  {
    counts.vendorInfoCount = 0;
    counts.vendorBinarySize = 0;
  }

  Tab<VkDeviceFaultAddressInfoEXT> addressInfos;
  Tab<VkDeviceFaultVendorInfoEXT> vendorInfos;
  Tab<char> vendorDump;

  addressInfos.resize(counts.addressInfoCount);
  vendorInfos.resize(counts.vendorInfoCount);
  vendorDump.resize(counts.vendorBinarySize);

  dump.addTagged(FaultReportDump::GlobalTag::TAG_EXT_FAULT, ID_COUNTS,
    String(64, "query reports %u-%u-%u records", counts.addressInfoCount, counts.vendorInfoCount, counts.vendorBinarySize));

  VkDeviceFaultInfoEXT info = {
    VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT, nullptr, "<unknown>", addressInfos.data(), vendorInfos.data(), vendorDump.data()};

  if (VULKAN_FAIL(vkDev.vkGetDeviceFaultInfoEXT(vkDev.get(), &counts, &info)))
  {
    dump.addTagged(FaultReportDump::GlobalTag::TAG_EXT_FAULT, ID_STATUS,
      String(128, "data with %u addresses, %u vendor specific data and %u bytes vendor dump can't be recived", counts.addressInfoCount,
        counts.vendorInfoCount, counts.vendorBinarySize));
    return;
  }

  dump.addTagged(FaultReportDump::GlobalTag::TAG_EXT_FAULT, ID_RECIVED,
    String(64, "recived %u-%u-%u records", counts.addressInfoCount, counts.vendorInfoCount, counts.vendorBinarySize));

  // counts content can be changed, avoid reading garbadge
  addressInfos.resize(counts.addressInfoCount);
  vendorInfos.resize(counts.vendorInfoCount);
  vendorDump.resize(counts.vendorBinarySize);

  dump.addTagged(FaultReportDump::GlobalTag::TAG_EXT_FAULT, ID_DESC, String(info.description));

  int idCounter = ID_REPORT_DATA;
  for (const auto &i : addressInfos)
    dump.addTagged(FaultReportDump::GlobalTag::TAG_EXT_FAULT, ++idCounter,
      String(64, "addr: type %u, val %u prec %u", i.addressType, i.reportedAddress, i.addressPrecision));

  for (const auto &i : vendorInfos)
    dump.addTagged(FaultReportDump::GlobalTag::TAG_EXT_FAULT, ++idCounter,
      String(64, "vendor: %s | %u-%u", i.description, i.vendorFaultCode, i.vendorFaultData));

  if (counts.vendorBinarySize)
  {
    FILE *f = fopen("vk_fault_vendor_dump.bin", "wb");
    if (f)
    {
      fwrite(vendorDump.data(), 1, counts.vendorBinarySize, f);
      fclose(f);
      dump.addTagged(FaultReportDump::GlobalTag::TAG_EXT_FAULT, ID_VENDOR_DUMP,
        String(64, "vendor dump of %u bytes saved to vk_fault_vendor_dump.bin", counts.vendorBinarySize));
    }
  }

  dump.addTagged(FaultReportDump::GlobalTag::TAG_EXT_FAULT, ID_STATUS, String("ok"));

#else // VK_EXT_device_fault
  G_ASSERTF(0, "vulkan: device fault: feature is available while it should not!");
#endif
}

void DeviceContext::generateFaultReportAtFrameEnd()
{
  VULKAN_LOCK_FRONT();
  front.replayRecord->generateFaultReport = true;
}

namespace
{
struct PipelineInfoDumpCb
{
  FaultReportDump &dump;

  PipelineInfoDumpCb(FaultReportDump &in_dump) : dump(in_dump) {}

  template <typename PipeType>
  void operator()(PipeType &pipe, ProgramID program)
  {
    String pipeStr("");
    auto shaderInfos = pipe.dumpShaderInfos();
    for (auto &info : shaderInfos)
    {
      if (info.name.empty() && info.debugName.empty())
        continue;

      pipeStr += "primary name: ";
      pipeStr += info.name;
      pipeStr += "\n";
      pipeStr += "debug name: ";
      pipeStr += info.debugName;
      pipeStr += "\n";
    }
    if (pipeStr.empty())
      pipeStr = "unknown pipeline";

    dump.addTagged(FaultReportDump::GlobalTag::TAG_PIPE, program.get(), pipeStr);
  }
};
} // namespace

void DeviceContext::generateFaultReport()
{
  debug("vulkan: GPU fault or invalid API command stream detected, generating report");

  FaultReportDump dump;

  {
    debug("vulkan: === state dump");

    debug("\n\nvulkan: ==== front pipelineState");
    front.pipelineState.dumpLog();
    debug("\n\nvulkan: ==== back pipelineState");
    back.pipelineState.dumpLog();
    debug("\n\nvulkan: ==== back executionState");
    back.executionState.dumpLog();
  }

  {
    debug("vulkan: === objects dump");
    reportAlliveObjects(dump);
  }

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
  {
    debug("vulkan: === pipe dump");

    PipelineManager &pipeMan = get_device().pipeMan;

    pipeMan.enumerate<ComputePipeline>(PipelineInfoDumpCb{dump});
    pipeMan.enumerate<VariatedGraphicsPipeline>(PipelineInfoDumpCb{dump});
#if D3D_HAS_RAY_TRACING
    pipeMan.enumerate<RaytracePipeline>(PipelineInfoDumpCb{dump});
#endif
  }
#endif

  {
    debug("vulkan: === execution markers dump");
    device.execMarkers.check();
    device.execMarkers.dumpFault(dump);
  }

  {
    debug("vulkan: === cpu replay dump");
    // only called when we are about to crash on the worker thread, so back member is owned by the
    // executing thread by default
    device.timelineMan.get<TimelineManager::CpuReplay>().enumerate([&](size_t index, const RenderWork &ctx) //
      {
        dump.addTagged(FaultReportDump::GlobalTag::TAG_WORK_ITEM, ctx.id, String(32, "work item ring index %u", index));
        ctx.dumpData(dump);
        // FIXME: hack, debugCommands impl depends on non const render work
        const_cast<RenderWork &>(ctx).dumpCommands(dump);
      });
  }

  {
    debug("vulkan: === device fault ext dump");
    dumpDeviceFaultExtData(device, dump);
  }

  dump.dumpLog();
}
