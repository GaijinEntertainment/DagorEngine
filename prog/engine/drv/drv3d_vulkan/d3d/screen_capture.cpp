// Copyright (C) Gaijin Games KFT.  All rights reserved.

// screen capture API impl
#include <drv/3d/dag_driver.h>
#include "buffer_resource.h"
#include <generic/dag_tab.h>
#include "swapchain.h"
#include "globals.h"
#include "device_context.h"
#include <image/dag_texPixel.h>
#include "global_lock.h"

using namespace drv3d_vulkan;

namespace
{

Buffer *captureBuffer = nullptr;
Tab<uint8_t> captureData;

} // namespace

void *d3d::fast_capture_screen(int &w, int &h, int &stride_bytes, int &format)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();

  VkExtent2D extent = Frontend::swapchain.getMode().extent;
  FormatStore fmt = Frontend::swapchain.getMode().format;
  uint32_t bufferSize = fmt.calculateSlicePich(extent.width, extent.height);

  w = extent.width;
  h = extent.height;
  stride_bytes = fmt.calculateRowPitch(extent.width);
  format = CAPFMT_X8R8G8B8;

  D3D_CONTRACT_ASSERTF(!captureBuffer, "vulkan: must call d3d::end_fast_capture_screen before calling d3d::fast_capture_screen again");
  captureBuffer = Buffer::create(bufferSize, DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER, 1, BufferMemoryFlags::TEMP);
  Globals::Dbg::naming.setBufName(captureBuffer, "screen capture staging buf");

  VkFormat swapchainVkFormat = VK_FORMAT_UNDEFINED;
  CmdCaptureScreen cmd{captureBuffer, Frontend::swapchain.getCurrentTarget(), &swapchainVkFormat};
  Globals::ctx.dispatchCommand(cmd);
  TIME_PROFILE(vulkan_fast_capture_screen);
  Globals::ctx.wait();
  captureBuffer->markNonCoherentRangeLoc(0, bufferSize, false);
  uint8_t *data = captureBuffer->ptrOffsetLoc(0);
  if (swapchainVkFormat == VK_FORMAT_R8G8B8A8_UNORM)
  {
    for (unsigned int pos = 0; pos < h * stride_bytes; pos += 4)
    {
      uint8_t tmp = data[pos];
      data[pos] = data[pos + 2];
      data[pos + 2] = tmp;
    }
  }
  else
  {
    D3D_CONTRACT_ASSERTF(swapchainVkFormat == VK_FORMAT_B8G8R8A8_UNORM,
      "vulkan: unexpected swapchain format %s, must be mapped to CAPFMT_X8R8G8B8",
      FormatStore::fromVkFormat(swapchainVkFormat).getNameString());
  }
  return data;
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
