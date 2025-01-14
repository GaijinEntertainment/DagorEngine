// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "constants.h"
#include "driver.h"
#include "extents.h"
#include "host_device_shared_memory_region.h"
#include "pipeline.h"
#include "tagged_types.h"

#include <drv/3d/rayTrace/dag_drvRayTrace.h>
#include <value_range.h>
#include <EASTL/fixed_vector.h>


namespace drv3d_dx12
{
class Image;

struct ClearDepthStencilValue
{
  float depth;
  uint32_t stencil;
};

union ClearColorValue
{
  float float32[4];
  uint32_t uint32[4];
};

struct ImageSubresourceRange
{
  MipMapRange mipMapRange;
  ArrayLayerRange arrayLayerRange;
};

struct ImageSubresourceLayers
{
  MipMapIndex mipLevel;
  ArrayLayerIndex baseArrayLayer;
};

struct ImageBlit
{
  ImageSubresourceLayers srcSubresource;
  Offset3D srcOffsets[2]; // change to 2d have no 3d blitting
  ImageSubresourceLayers dstSubresource;
  Offset3D dstOffsets[2]; // change to 2d have no 3d blitting
};

struct ImageCopy
{
  SubresourceIndex srcSubresource;
  SubresourceIndex dstSubresource;
  Offset3D dstOffset;
  D3D12_BOX srcBox;
};

struct ImageStateRange
{
  Image *image = nullptr;
  uint32_t stateArrayIndex = 0;
  ValueRange<uint16_t> arrayRange;
  ValueRange<uint16_t> mipRange;
};
struct ImageStateRangeInfo
{
  D3D12_RESOURCE_STATES state;
  ValueRange<uint32_t> idRange;
};

struct BufferCopy
{
  uint64_t srcOffset;
  uint64_t dstOffset;
  uint64_t size;
};

struct BufferImageCopy
{
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
  UINT subresourceIndex;
  Offset3D imageOffset;
};

struct BufferStateInfo
{
  D3D12_RESOURCE_STATES state;
  uint32_t id;
};

struct FrontendTimelineTextureUsage
{
  Image *texture = nullptr;
  uint32_t subResId = 0;
  D3D12_RESOURCE_STATES usage = D3D12_RESOURCE_STATE_COMMON;
  D3D12_RESOURCE_STATES nextUsage = D3D12_RESOURCE_STATE_COMMON;
  D3D12_RESOURCE_STATES prevUsage = D3D12_RESOURCE_STATE_COMMON;
  size_t nextIndex = ~size_t(0);
  size_t prevIndex = ~size_t(0);
  size_t cmdIndex = 0;
  // for render/depth-stencil target this is the index to the last draw
  // command before it transitions into nextUsage.
  // This information is used to insert split barriers.
  size_t lastCmdIndex = ~size_t(0);
};

struct HostDeviceSharedMemoryImageCopyInfo
{
  HostDeviceSharedMemoryRegion cpuMemory;
  Image *image;
  uint32_t copyIndex;
  uint32_t copyCount;
};

#if D3D_HAS_RAY_TRACING
struct RaytraceGeometryDescriptionBufferResourceReferenceSet
{
  BufferResourceReference vertexOrAABBBuffer;
  BufferResourceReference indexBuffer;
  BufferResourceReference transformBuffer;
};
#endif

using TextureMipsCopyInfo = eastl::fixed_vector<BufferImageCopy, MAX_MIPMAPS, false>;
using RootConstatInfo = eastl::fixed_vector<uint32_t, MAX_ROOT_CONSTANTS, false>;

} // namespace drv3d_dx12