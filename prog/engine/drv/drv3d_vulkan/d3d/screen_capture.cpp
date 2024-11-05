// Copyright (C) Gaijin Games KFT.  All rights reserved.

// screen capture API impl
#include <drv/3d/dag_driver.h>
#include "buffer_resource.h"
#include <generic/dag_tab.h>
#include "swapchain.h"
#include "globals.h"
#include "device_context.h"
#include <image/dag_texPixel.h>

using namespace drv3d_vulkan;

namespace
{

Buffer *captureBuffer = nullptr;
Tab<uint8_t> captureData;

} // namespace

void *d3d::fast_capture_screen(int &w, int &h, int &stride_bytes, int &format)
{
  VkExtent2D extent = Globals::swapchain.getMode().extent;
  FormatStore fmt = Globals::swapchain.getMode().colorFormat;
  uint32_t bufferSize = fmt.calculateSlicePich(extent.width, extent.height);

  w = extent.width;
  h = extent.height;
  stride_bytes = fmt.calculateRowPitch(extent.width);
  format = CAPFMT_X8R8G8B8;

  G_ASSERTF(!captureBuffer, "vulkan: must call d3d::end_fast_capture_screen before calling d3d::fast_capture_screen again");
  captureBuffer = Buffer::create(bufferSize, DeviceMemoryClass::HOST_RESIDENT_HOST_READ_ONLY_BUFFER, 1, BufferMemoryFlags::TEMP);
  Globals::Dbg::naming.setBufName(captureBuffer, "screen capture staging buf");

  CmdCaptureScreen cmd{captureBuffer};
  Globals::ctx.dispatchCommand(cmd);
  TIME_PROFILE(vulkan_fast_capture_screen);
  Globals::ctx.wait();
  captureBuffer->markNonCoherentRangeLoc(0, bufferSize, false);
  return captureBuffer->ptrOffsetLoc(0);
}

void d3d::end_fast_capture_screen()
{
  Globals::ctx.destroyBuffer(captureBuffer);
  captureBuffer = nullptr;
}

TexPixel32 *d3d::capture_screen(int &w, int &h, int &stride_bytes)
{
  int fmt;
  void *ptr = fast_capture_screen(w, h, stride_bytes, fmt);
  captureData.resize(w * h * sizeof(TexPixel32));
  if (CAPFMT_X8R8G8B8 == fmt)
  {
    memcpy(captureData.data(), ptr, data_size(captureData));
  }
  else
    G_ASSERT(false);
  end_fast_capture_screen();
  return (TexPixel32 *)captureData.data();
}

void d3d::release_capture_buffer() { clear_and_shrink(captureData); }
