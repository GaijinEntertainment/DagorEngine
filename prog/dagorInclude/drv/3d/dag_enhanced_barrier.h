//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <util/dag_bitFlagsMask.h>
#include <generic/dag_span.h>
#include <drv/3d/dag_consts.h>

class Sbuffer;
class BaseTexture;

namespace d3d
{

enum class TextureLayout : uint32_t
{
  // Transitions from UNDEFINED to anything else always cause a discard
  // (other usages of this transition are pointless)
  // This makes this type of barrier equivalent to activating a placed
  // resource. It's also mandatory to dispatch this type of barrier
  // before a placed texture is first used. Must be paired with NoAccess.
  Undefined,

  // Sometimes, situations in daBfg occur where we can replace a few
  // RO->RO barriers with a single GenericRead layout barrier, which
  // can make perf better due to less pipeline stalls and better SP
  // occupancy.
  GenericRead,

  // RTV
  RenderTarget,
  // UAV
  UnorderedAccess,
  // DSV (a bit overly detailed to cover all cases in vk/dx12)
  DepthRwStencilRw,
  DepthRwStencilRo,
  DepthRoStencilRw,
  DepthRoStencilRo,
  DepthRw,
  DepthRo,

  // SRV
  ShaderResource,

  CopySource,
  CopyDest,

  // Need to distinguish between them as on dx blits are implemented
  // through shaders and on vk they are natively supported
  BlitSource,
  BlitDest,

  ResolveSource,
  ResolveDest,

  // VRS rate texture
  ShadingRateSource
};

enum class PipelineStageFlag : uint32_t
{
  NoStage = 0u,

  // Reading an indirect command buffer
  ExecuteIndirect = 1u << 0,

  // IB read access
  IndexInput = 1u << 1,
  // VB read access
  VertexAttributeInput = 1u << 2,
  // Vertex, geometry, tessellation, task and mesh shaders
  AllVertexShading = 1u << 3,
  // Specifically for VRS rate texture usage
  // (or fragment density map on vk if we ever need it)
  Rasterization = 1u << 4,
  // Early depth/stencil tests
  EarlyFragmentTests = 1u << 5,
  // Pixel/fragment shader
  PixelShading = 1u << 6,
  // Late depth/stencil tests
  LateFragmentTests = 1u << 7,
  // RT output, blending, etc. Also, surprisingly, d3d::clearview.
  OutputMerging = 1u << 8,

  // Compute shader
  ComputeShading = 1u << 9,

  Copy = 1u << 12,
  Blit = 1u << 13,

  // Any clear that operates on an arbitrary resource.
  // Note that d3d::clearview is NOT part of this stage,
  // it is part of the OutputMerging stage.
  Clear = 1u << 14,

  // HW MSAA resolve
  Resolve = 1u << 15,

  // All commands
  All = 1u << 31,
};

using PipelineStageFlags = BitFlagsMask<PipelineStageFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(PipelineStageFlag);

enum class AccessFlag : uint32_t
{
  NoAccess = 0,

  // Buffer for draw/dispatch indirect (RO)
  IndirectArgument = 1u << 1,

  // VBV (RO)
  VertexBuffer = 1u << 2,

  // IBV (RO)
  IndexBuffer = 1u << 3,

  // CBV (RO)
  ConstantBuffer = 1u << 4,

  // RTV
  RenderTargetRead = 1u << 5,
  RenderTargetWrite = 1u << 6,

  // UAV (RW)
  UnorderedAccess = 1u << 7,

  // DSV
  DepthStencilWrite = 1u << 8,
  DepthStencilRead = 1u << 9,

  // For subpass dependencies
  InputAttachment = 1u << 10,

  // SRV (RO)
  ShaderResource = 1u << 11,

  CopyRead = 1u << 12,
  CopyWrite = 1u << 13,

  BlitRead = 1u << 14,
  BlitWrite = 1u << 15,

  ResolveRead = 1u << 16,
  ResolveWrite = 1u << 17,

  // VRS rate texture (RO)
  ShadingRate = 1u << 23
};

using AccessFlags = BitFlagsMask<AccessFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(AccessFlag);

struct Range
{
  uint32_t first;
  uint32_t count;
};

struct TextureSubresourceRange
{
  Range mips;
  Range layers;
  Range planes;
};

struct PipelineSyncOperation
{
  PipelineStageFlags src;
  PipelineStageFlags dst;
};

struct MemorySyncOperation
{
  AccessFlags src;
  AccessFlags dst;
};

struct LayoutTransitionOperation
{
  TextureLayout src;
  TextureLayout dst;
};

struct TextureBarrier
{
  PipelineSyncOperation pipelineSync;
  MemorySyncOperation memorySync;
  LayoutTransitionOperation layoutTransition;

  // later on we can add srcQueue and dstQueue and
  // while we don't have explicit command lists
  // implementations can chose the queue to execute on
  // based on the srcQueue field.

  TextureSubresourceRange subresources;
};

struct BufferBarrier
{
  PipelineSyncOperation pipelineSync;
  MemorySyncOperation memorySync;

  // later on we can add srcQueue and dstQueue and
  // while we don't have explicit command lists
  // implementations can chose the queue to execute on
  // based on the srcQueue field.

  // Note that barriers on parts of a buffer are not supported,
  // as according to microsoft they are underspecified even in vulkan,
  // and it's not clear whether they can have any perf benefit at all.
};

struct TextureBarrierBatchItem
{
  TextureBarrier barrier;
  BaseTexture *texture;
};

struct BufferBarrierBatchItem
{
  BufferBarrier barrier;
  Sbuffer *buffer;
};

#if !_TARGET_D3D_MULTI
void enhanced_texture_barrier(const TextureBarrier &barrier, BaseTexture *texture);
void enhanced_buffer_barrier(const BufferBarrier &barrier, Sbuffer *buffer);
// Will check performance and remove the batch versions if they don't provide any benefit.
// Otherwise, remove the non-batch versions and use these exclusively.
void enhanced_barrier_batch(dag::ConstSpan<TextureBarrierBatchItem> texture_barriers,
  dag::ConstSpan<BufferBarrierBatchItem> buffer_barriers);
#endif

} // namespace d3d
