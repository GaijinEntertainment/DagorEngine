// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "buffer_resource.h"
#include "util/fault_report.h"

namespace drv3d_vulkan
{

struct CommandDebugInfo
{
  uint32_t commandIndex;
  uint32_t workItemIndex;
  const void *commandPtr;
  bool passed;
};

class ExecutionMarkers
{
#if VK_AMD_buffer_marker
  Buffer *executionMarkerBuffer = nullptr;
#endif

  uint32_t commandIndex = 0;
  eastl::vector<CommandDebugInfo> commandDebugData;

  void markAsPassedIfLessOrEquel(uint32_t id);

public:
  void init();
  void shutdown();
  void dumpFault(FaultReportDump &dump);

  void check();
  bool hasPassed(size_t id, const void *ptr);
  void write(VkPipelineStageFlagBits stage, size_t work_idx, const void *cmd);
};

} // namespace drv3d_vulkan