// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// provides limiting logic for resource upload buffer
// we need to avoid growing it too much over N frames
// and provide always-allow path for first texture on frame, to guarantie
// multiple mips/slices upload on low budget cases

#include <drv/3d/dag_tex3d.h>
#include <atomic>
#include <util/dag_stdint.h>

namespace drv3d_vulkan
{

struct ResourceUploadLimit
{
  void reset();
  bool consume(uint32_t size, BaseTexture *bt);
  // controls consume logic to always no fail or work arbitrary on caller thread (thread local)
  // default is "arbitrary", i.e. no fail is disabled
  // note: this will lead to allocation over limit
  void setNoFailOnThread(bool val);

private:
  uint32_t peakSize = 0;
  std::atomic<uint32_t> allocatedSize{0};
  std::atomic<BaseTexture *> overallocTarget = nullptr;
};

} // namespace drv3d_vulkan
