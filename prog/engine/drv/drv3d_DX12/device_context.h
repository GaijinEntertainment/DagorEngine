// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "3d/dag_latencyTypes.h"
#include "bindless.h"
#include "buffer.h"
#include "command_list.h"
#include "command_stream_set.h"
#include "const_register_type.h"
#include "debug/command_list_logger.h"
#include "debug/device_context_state.h"
#include "debug/frame_command_logger.h"
#include "device_context_cmd.h"
#include "device_queue.h"
#include "events_pool.h"
#include "extra_data_arrays.h"
#include "frame_buffer.h"
#include "fsr_args.h"
#include "gpu_engine_state.h"
#include "info_types.h"
#include "query_manager.h"
#include "resource_memory_heap.h"
#include "resource_state_tracker.h"
#include "forward_ring.h"
#include "stacked_profile_events.h"
#include "stateful_command_buffer.h"
#include "streamline_adapter.h"
#include "swapchain.h"
#include "synced_command_store.h"
#include "tagged_handles.h"
#include "texture.h"
#include "viewport_state.h"
#include "xess_wrapper.h"
#include "fsr_wrapper.h"

#if USE_DLSS_WITHOUT_STREAMLINE
#include "dlss.h"
#endif

#include <dag/dag_vector.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_renderPass.h>
#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_threads.h>
#include <gpuVendorNvidia.h>


struct FrameEvents;

namespace drv3d_dx12
{
class RayTracePipeline;
class Device;
#if D3D_HAS_RAY_TRACING
struct RaytraceAccelerationStructure;
#endif

#if _TARGET_XBOX
#include "device_context_xbox.h"
#endif

constexpr uint32_t timing_history_length = 32;

inline ImageCopy make_whole_resource_copy_info()
{
  ImageCopy result{};
  result.srcSubresource = SubresourceIndex::make(~uint32_t{0});
  return result;
}

inline bool is_whole_resource_copy_info(const ImageCopy &copy) { return copy.srcSubresource == SubresourceIndex::make(~uint32_t{0}); }

BufferImageCopy calculate_texture_subresource_copy_info(const Image &texture, uint32_t subresource_index = 0, uint64_t offset = 0);

TextureMipsCopyInfo calculate_texture_mips_copy_info(const Image &texture, uint32_t mip_levels, uint32_t array_slice = 0,
  uint32_t array_size = 1, uint64_t initial_offset = 0);

class ReadBackManager
{
  struct BufferReadBackRecord
  {
    BufferResourceReferenceAndRange source;
    HostDeviceSharedMemoryRegion destination;
    uint64_t offset;
  };
  struct TextureReadBackRecord
  {
    Image *image;
    HostDeviceSharedMemoryRegion targetMemory;
    BufferImageCopy copyInfo;
  };
  struct BufferFrameRef
  {
    uint32_t bufferID;
    ValueRange<uint64_t> subRange;
    uint64_t frameIndex;
  };
  struct ImageFrameRef
  {
    Image *image;
    uint64_t frameIndex;
  };
  struct TextureMoveRecord
  {
    Image *from;
    Image *to;
  };
  struct BufferMoveRecord
  {
    BufferResourceReferenceAndRange from;
    BufferResourceReferenceAndOffset to;
  };
  dag::Vector<TextureReadBackRecord> textureReadBackRecords;
  dag::Vector<BufferReadBackRecord> bufferReadBackRecords;
  dag::Vector<ImageFrameRef> imageReadBackFrames;
  dag::Vector<BufferFrameRef> bufferReadBackFrames;
  dag::Vector<TextureMoveRecord> textureMoveRecords;
  dag::Vector<BufferMoveRecord> bufferMoveRecords;
  uint64_t nextSync = 0;

  void updateProgressFor(Image *image, uint64_t frame_progress)
  {
    auto at =
      eastl::find_if(imageReadBackFrames.begin(), imageReadBackFrames.end(), [image](auto &info) { return image == info.image; });
    if (at != imageReadBackFrames.end())
    {
      nextSync = max(at->frameIndex, nextSync);
      at->frameIndex = frame_progress;
    }
    else
    {
      ImageFrameRef info;
      info.image = image;
      info.frameIndex = frame_progress;
      imageReadBackFrames.push_back(info);
    }
  }
  void updateProgressFor(BufferResourceReferenceAndRange buffer, uint64_t frame_progress)
  {
    auto at = eastl::find_if(bufferReadBackFrames.begin(), bufferReadBackFrames.end(),
      [bufferID = buffer.resourceId.index(), subRange = make_value_range(buffer.offset, buffer.size)](auto &info) {
        return bufferID == info.bufferID && subRange == info.subRange;
      });
    if (at != bufferReadBackFrames.end())
    {
      nextSync = max(at->frameIndex, nextSync);
      at->frameIndex = frame_progress;
    }
    else
    {
      BufferFrameRef info;
      info.bufferID = buffer.resourceId.index();
      info.subRange = make_value_range(buffer.offset, buffer.size);
      info.frameIndex = frame_progress;
      bufferReadBackFrames.push_back(info);
    }
  }

public:
  void addReadBack(BufferResourceReferenceAndRange buffer, HostDeviceSharedMemoryRegion cpu_memory, uint64_t offset)
  {
    BufferReadBackRecord record;
    record.source = buffer;
    record.destination = cpu_memory;
    record.offset = offset;
    bufferReadBackRecords.push_back(record);
  }
  void addReadBack(Image *image, HostDeviceSharedMemoryRegion cpu_memory, BufferImageCopyListRef::RangeType regions)
  {
    for (auto &&region : regions)
    {
      TextureReadBackRecord record;
      record.image = image;
      record.targetMemory = cpu_memory;
      record.copyInfo = region;
      textureReadBackRecords.push_back(record);
    }
  }
  // NOTE: this is only needed when we allow <write> -> <read back> -> <write> to resources
  // right now this is a possible use case with the DX11 implementation but was not observed yet
  void onTextureWriteAccess(ResourceUsageManagerWithHistory &resource_state, BarrierBatcher &barrier_batcher,
    SplitTransitionTracker &split_transition_tracker, StatefulCommandBuffer &command_list, Image *image)
  {
    // are any read backs pending for the texture?
    auto firstEntry = eastl::find_if(textureReadBackRecords.begin(), textureReadBackRecords.end(),
      [image](auto &info) { return image == info.image; });
    if (textureReadBackRecords.end() == firstEntry)
    {
      return;
    }

    // we need to generate all barriers first
    auto at = firstEntry;
    do
    {
      resource_state.useTextureAsCopySource(barrier_batcher, split_transition_tracker, image,
        SubresourceIndex::make(at->copyInfo.subresourceIndex));

      at = eastl::find_if(at + 1, textureReadBackRecords.end(), [image](auto &info) { return image == info.image; });
    } while (at != textureReadBackRecords.end());

    barrier_batcher.execute(command_list);

    // now we generate all read back copy commands and remove the entries
    D3D12_TEXTURE_COPY_LOCATION src;
    D3D12_TEXTURE_COPY_LOCATION dst;
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    at = firstEntry;
    do
    {
      src.pResource = at->image->getHandle();
      src.SubresourceIndex = at->copyInfo.subresourceIndex;

      dst.pResource = at->targetMemory.buffer;
      dst.PlacedFootprint = at->copyInfo.layout;
      dst.PlacedFootprint.Offset += at->targetMemory.range.front();

      // TODO: remove offset
      G_ASSERT(at->copyInfo.imageOffset.x == 0 && at->copyInfo.imageOffset.y == 0 && at->copyInfo.imageOffset.z == 0);
      command_list.copyTexture(&dst, 0, 0, 0, &src, nullptr);

      // swap with back and pop back -> fast remove
      eastl::swap(*at, textureReadBackRecords.back());
      textureReadBackRecords.pop_back();

      at = eastl::find_if(at, textureReadBackRecords.end(), [image](auto &info) { return image == info.image; });
    } while (at != textureReadBackRecords.end());
  }
  // NOTE: this is only needed when we allow <write> -> <read back> -> <write> to resources
  // right now this is a possible use case with the DX11 implementation but was not observed yet
  void onBufferWriteAccess(ResourceUsageManagerWithHistory &resource_state, BarrierBatcher &barrier_batcher,
    StatefulCommandBuffer &command_list, BufferResourceReference buffer)
  {
    auto at = eastl::find_if(bufferReadBackRecords.begin(), bufferReadBackRecords.end(),
      [&buffer](auto &info) { return buffer == info.source; });

    if (bufferReadBackRecords.end() == at)
    {
      return;
    }

    // no need to walk the read back list as we only can have one barrier per unique buffer
    resource_state.useBufferAsCopySource(barrier_batcher, buffer);
    barrier_batcher.execute(command_list);

    do
    {
      auto dstOffset = at->destination.range.front() + at->offset;
      auto srcOffset = at->source.offset;

      command_list.copyBuffer(at->destination.buffer, dstOffset, at->source.buffer, srcOffset, at->source.size);

      eastl::swap(*at, bufferReadBackRecords.back());
      bufferReadBackRecords.pop_back();

      at = eastl::find_if(bufferReadBackRecords.begin(), bufferReadBackRecords.end(),
        [&buffer](auto &info) { return buffer == info.source; });
    } while (at != bufferReadBackRecords.end());
  }
  // frame_progress is the current frame progress, this is used to limit on how many frames of the past we wait
  // this has to be called before 'flushToReadBackQueue' of this frame, as 'flushToReadBackQueue' generates the
  // sync data for the next frame.
  uint64_t syncFrameBeginWith(uint64_t frame_progress)
  {
    // limit how may frames without sync can pass to keep dependency chains short
    constexpr uint64_t max_sync_delta = 4;
    const uint64_t delta = min(frame_progress - nextSync, max_sync_delta);
    // on move sync we need to sync with prev frame to guarantee that the moved resources are ready
    const uint64_t syncPoint = frame_progress - delta;

    auto bufferReadBackFrameNewEnd = eastl::remove_if(bufferReadBackFrames.begin(), bufferReadBackFrames.end(),
      [syncPoint](auto &info) { return info.frameIndex <= syncPoint; });
    bufferReadBackFrames.erase(bufferReadBackFrameNewEnd, bufferReadBackFrames.end());

    auto imageReadBackFramesNewEnd = eastl::remove_if(imageReadBackFrames.begin(), imageReadBackFrames.end(),
      [syncPoint](auto &info) { return info.frameIndex <= syncPoint; });
    imageReadBackFrames.erase(imageReadBackFramesNewEnd, imageReadBackFrames.end());

    // need to update nextSync otherwise it may stay forever at 0
    nextSync = syncPoint;

    return syncPoint;
  }
  bool hasAnyReadBacksQueued() const
  {
    return !(
      textureReadBackRecords.empty() && bufferReadBackRecords.empty() && textureMoveRecords.empty() && bufferMoveRecords.empty());
  }
  // this generates the barriers for the current frame, to make the read back resources read to be read back.
  // records the read back commands into a read back command buffer and primes the data structures for syncing
  // for the next frame.
  void flushToReadBackQueue(ResourceUsageManagerWithHistory &resource_state, BarrierBatcher &barrier_batcher,
    SplitTransitionTracker &split_transition_tracker, CopyCommandList<VersionedPtr<D3DCopyCommandList>> &copy_command_list,
    uint64_t frame_progress, bool has_whole_copy_hang_bug)
  {
    D3D12_TEXTURE_COPY_LOCATION src;
    D3D12_TEXTURE_COPY_LOCATION dst;
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    for (auto &textureReadBack : textureReadBackRecords)
    {
      updateProgressFor(textureReadBack.image, frame_progress);

      // Workaround: transition ALL subresources to COMMON, not just the one being read back.
      // Per D3D12 spec: "subresources used in Copy queues MUST be in the state D3D12_RESOURCE_STATE_COMMON".
      // We only need to copy one subresource, but transitioning just that subresource causes gpu validation error:
      // ID3D12CommandQueue1::ExecuteCommandLists: Using ResourceBarrier on Command List (0x000001E98DB225C0:'Unnamed
      // ID3D12GraphicsCommandList Object'): Before state (0x4: D3D12_RESOURCE_STATE_RENDER_TARGET) of resource
      // (0x000001E9F7BF4C30:'clear_color_renderTarget') (subresource: 1) specified by transition barrier does not match with the state
      // (0x0: D3D12_RESOURCE_STATE_[COMMON|PRESENT]) specified in preceding ResourceBarrier or as InitialState
      resource_state.useTextureAsCopySourceForWholeCopyOnReadbackQueue(barrier_batcher, split_transition_tracker,
        textureReadBack.image);

      src.pResource = textureReadBack.image->getHandle();
      src.SubresourceIndex = textureReadBack.copyInfo.subresourceIndex;

      dst.pResource = textureReadBack.targetMemory.buffer;
      dst.PlacedFootprint = textureReadBack.copyInfo.layout;
      dst.PlacedFootprint.Offset += textureReadBack.targetMemory.range.front();

      // TODO: remove offset
      G_ASSERT(textureReadBack.copyInfo.imageOffset.x == 0 && textureReadBack.copyInfo.imageOffset.y == 0 &&
               textureReadBack.copyInfo.imageOffset.z == 0);
      copy_command_list.copyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    }
    textureReadBackRecords.clear();

    for (auto &textureMoveRecord : textureMoveRecords)
    {
      resource_state.useTextureAsCopySourceForWholeCopyOnReadbackQueue(barrier_batcher, split_transition_tracker,
        textureMoveRecord.from);
      resource_state.useTextureAsCopyDestinationForWholeCopyOnReadbackQueue(barrier_batcher, split_transition_tracker,
        textureMoveRecord.to);
    }
    if (has_whole_copy_hang_bug)
    {
      dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
      for (auto &textureMoveRecord : textureMoveRecords)
      {
        for (const auto subres : textureMoveRecord.from->getSubresourceRange())
        {
          src.pResource = textureMoveRecord.from->getHandle();
          src.SubresourceIndex = subres.index();

          dst.pResource = textureMoveRecord.to->getHandle();
          dst.SubresourceIndex = subres.index();

          copy_command_list.copyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        }
      }
    }
    else
    {
      for (auto &textureMoveRecord : textureMoveRecords)
      {
        copy_command_list.copyResource(textureMoveRecord.to->getHandle(), textureMoveRecord.from->getHandle());
      }
    }

    for (auto &bufferReadBack : bufferReadBackRecords)
    {
      updateProgressFor(bufferReadBack.destination, frame_progress);

      auto dstOffset = bufferReadBack.destination.range.front() + bufferReadBack.offset;
      auto srcOffset = bufferReadBack.source.offset;
      // only need to do read backs on read back command buffer as buffers decay to common state
      copy_command_list.copyBufferRegion(bufferReadBack.destination.buffer, dstOffset, bufferReadBack.source.buffer, srcOffset,
        bufferReadBack.source.size);
    }
    bufferReadBackRecords.clear();

    for (auto &bufferMoveRecord : bufferMoveRecords)
    {
      copy_command_list.copyBufferRegion(bufferMoveRecord.to.buffer, bufferMoveRecord.to.offset, bufferMoveRecord.from.buffer,
        bufferMoveRecord.from.offset, bufferMoveRecord.from.size);
    }
    if (!textureMoveRecords.empty() || !bufferMoveRecords.empty())
    {
      nextSync = frame_progress;
    }
    textureMoveRecords.clear();
    bufferMoveRecords.clear();
  }
  // swapchain read backs are special, we have to do it on the graphics queue before present
  void doSwapchainReadBack(ResourceUsageManagerWithHistory &resource_state, BarrierBatcher &barrier_batcher,
    SplitTransitionTracker &split_transition_tracker, StatefulCommandBuffer &command_list);
  // currently we execute moves on the read back queue because it was convenient to do this way (lot of pluming was ready for it and
  // the scheduling ideal for it) so the manager will handle this, if we are recording moves we can no longer overlap read backs with
  // future frames, as they need to move to be completed this may be changed in the future
  void moveTexture(Image *from, Image *to)
  {
    // to can not be a source for read backs as its still owned by the resource manager and its not publicly accessible yet
    // from could be a source for a read back, but in this case the state would be the same so nothing to handle here
    TextureMoveRecord record;
    record.from = from;
    record.to = to;
    textureMoveRecords.push_back(record);
  }
  void moveBuffer(BufferResourceReferenceAndRange from, BufferResourceReferenceAndOffset to)
  {
    BufferMoveRecord record;
    record.from = from;
    record.to = to;
    bufferMoveRecords.push_back(record);
  }
#if _TARGET_SCARLETT
  // Workaround for an Xbox issue with non-zero texture array layers readbacks.
  void flushArrayLayersReadbacksToGraphicsQueue(ResourceUsageManagerWithHistory &resource_state, BarrierBatcher &barrier_batcher,
    SplitTransitionTracker &split_transition_tracker, StatefulCommandBuffer &command_list)
  {
    if (!command_list.isReadyForRecording())
    {
      return;
    }

    auto partitionPoint = eastl::partition(textureReadBackRecords.begin(), textureReadBackRecords.end(), [](const auto &readback) {
      uint32_t arrayLayer = calculate_array_slice_from_index(readback.copyInfo.subresourceIndex,
        readback.image->getMipLevelRange().count(), readback.image->getArrayLayerRange().count());
      return arrayLayer == 0;
    });

    if (partitionPoint == textureReadBackRecords.end())
      return;

    D3D12_TEXTURE_COPY_LOCATION src;
    D3D12_TEXTURE_COPY_LOCATION dst;
    src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    for (auto it = partitionPoint; it != textureReadBackRecords.end(); ++it)
    {
      resource_state.useTextureAsCopySource(barrier_batcher, split_transition_tracker, it->image,
        SubresourceIndex::make(it->copyInfo.subresourceIndex));

      barrier_batcher.execute(command_list);

      src.pResource = it->image->getHandle();
      src.SubresourceIndex = it->copyInfo.subresourceIndex;

      dst.pResource = it->targetMemory.buffer;
      dst.PlacedFootprint = it->copyInfo.layout;
      dst.PlacedFootprint.Offset += it->targetMemory.range.front();

      // TODO: remove offset
      G_ASSERT(it->copyInfo.imageOffset.x == 0 && it->copyInfo.imageOffset.y == 0 && it->copyInfo.imageOffset.z == 0);
      command_list.copyTexture(&dst, 0, 0, 0, &src, nullptr);
    }

    textureReadBackRecords.erase(partitionPoint, textureReadBackRecords.end());
  }
#endif
};

class TextureConcurrentTextureCopyManager
{
  struct TextureUploadRecord
  {
    Image *target = nullptr;
    Image *sourceTexture = nullptr;
    SubresourceIndex textureCopyDestSubresource;
    union
    {
      struct
      {
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
        Offset3D imageOffset;
        HostDeviceSharedMemoryRegion source;
      };
      struct
      {
        SubresourceIndex textureCopySourceSubresource;
      };
    };
    // counts how many followup copies depend on target.
    uint32_t dependencyCount : 31 = 0;
    // has it at least one parent
    uint32_t hasParent : 1 = 0;

    bool targetIsSourceOf(const TextureUploadRecord &other) const
    {
      return target == other.sourceTexture && getDestSubresourceIndex() == other.getSourceSubresourceIndex();
    }

    void copyFromSameSource(TextureUploadRecord &other) const
    {
      if (sourceTexture)
      {
        other.sourceTexture = sourceTexture;
        other.textureCopySourceSubresource = textureCopySourceSubresource;
      }
      else
      {
        other.sourceTexture = nullptr;
        other.layout = layout;
        other.imageOffset = imageOffset;
        other.source = source;
      }
      other.hasParent = hasParent;
    }

    SubresourceIndex getDestSubresourceIndex() const { return textureCopyDestSubresource; }

    SubresourceIndex getSourceSubresourceIndex() const
    {
      return sourceTexture ? textureCopySourceSubresource : SubresourceIndex::make(0);
    }

    D3D12_TEXTURE_COPY_LOCATION makeCopySourceLocation() const
    {
      D3D12_TEXTURE_COPY_LOCATION src;
      if (sourceTexture)
      {
        src = {
          .pResource = sourceTexture->getHandle(),
          .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
          .SubresourceIndex = textureCopySourceSubresource.index(),
        };
      }
      else
      {
        src = {
          .pResource = source.buffer,
          .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
          .PlacedFootprint = layout,
        };
        src.PlacedFootprint.Offset += source.range.front();
      }
      return src;
    }

    D3D12_TEXTURE_COPY_LOCATION makeCopyDestLocation() const
    {
      return {
        .pResource = target->getHandle(),
        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = textureCopyDestSubresource.index(),
      };
    }

    Offset3D getCopyDestOffset() const { return sourceTexture ? Offset3D{.x = 0, .y = 0, .z = 0} : imageOffset; }

    bool isPartialCopy() const
    {
      if (sourceTexture)
      {
        // texture to texture is always full copy of the min of src and target size (which are supposed to be the same at all times)
        return false;
      }
      // when any offset is not 0, that it is 100% a partial copy
      if (imageOffset.x || imageOffset.y || imageOffset.z)
      {
        return true;
      }
      Extent3D blockExtent{1, 1, 1};
      target->getFormat().getBytesPerPixelBlock(&blockExtent.width, &blockExtent.height);

      const auto dstLevel = target->stateIndexToMipIndex(textureCopyDestSubresource);
      const auto dstExtent = align_value(target->getMipExtents(dstLevel), blockExtent);
      return dstExtent.width != layout.Footprint.Width || dstExtent.height != layout.Footprint.Height ||
             dstExtent.depth != layout.Footprint.Depth;
      // NOTE: For 3d textures this will always return true, as we never upload a full 3d texture (except when it has a depth of 1),
      // as we upload 3d textures in 2d images, always 1 depth layer at a time. So Footprint.Depth will never be anything other than 1.
    }

    eastl::optional<D3D12_BOX> getCopySourceBox() const
    {
      if (!sourceTexture)
      {
        return eastl::nullopt;
      }

      Extent3D blockExtent{1, 1, 1};
      sourceTexture->getFormat().getBytesPerPixelBlock(&blockExtent.width, &blockExtent.height);

      const auto dstLevel = target->stateIndexToMipIndex(textureCopyDestSubresource);
      const auto srcLevel = sourceTexture->stateIndexToMipIndex(textureCopySourceSubresource);

      const auto dstExtent = align_value(target->getMipExtents(dstLevel), blockExtent);
      const auto srcExtent = align_value(sourceTexture->getMipExtents(srcLevel), blockExtent);

      // If sizes do not match, we try to safe what we can and only copy a region that does not cause
      // a out of bounds read error and device reset.
      const auto copyExtent = min(dstExtent, srcExtent);
      // not reporting an error here when 'dstExtent != srcExtent', as it would be already when this copy command was generated

      return D3D12_BOX{
        .left = 0,
        .top = 0,
        .front = 0,
        .right = copyExtent.width,
        .bottom = copyExtent.height,
        .back = copyExtent.depth,
      };
    }
  };

  struct TextureSubresourceUsage
  {
    // bit count has to be power of 2 to avoid awkward rounding that breaks lots of assumptions
    static constexpr uint32_t sub_res_bits = 4;
    static constexpr uint32_t sub_res_per_byte = sizeof(uint8_t) * 8 / sub_res_bits;
    static constexpr uint32_t sub_res_available_bit_index = 0;
    static constexpr uint32_t sub_res_inline_barrier_bit_index = 1;
    static constexpr uint32_t sub_res_has_executed_bit_index = 2;
    static constexpr uint32_t sub_res_unused_bit_index = 3;
    // with 4 bits each, this allows for 28 consecutive sub resource
    static constexpr uint32_t sub_res_array_count = sizeof(uint64_t) - sizeof(uint16_t) + sizeof(uint64_t);
    Image *image = nullptr;
    // Enough for every case, max mip is 16 (bc max tex size is 16k), max arrays is 2k and max plane count is 2, which is 16 x 2k x 2
    // which is 65k
    uint16_t subResOffset = 0;
    uint8_t subResMaskArray[sub_res_array_count]{};

    static uint16_t calc_sub_res_offset(SubresourceIndex index) { return (index.index() / sub_res_array_count) / sub_res_per_byte; }
    struct ByteAndShiftIndex
    {
      uint8_t byteIndex;
      uint8_t shiftIndex;
    };
    ByteAndShiftIndex getByteAndShiftIndex(SubresourceIndex index) const
    {
      const auto relativeOffset = index.index() - (subResOffset * sub_res_array_count * sub_res_per_byte);
      return {
        .byteIndex = static_cast<uint8_t>(relativeOffset / sub_res_per_byte),
        .shiftIndex = static_cast<uint8_t>(relativeOffset % sub_res_per_byte),
      };
    }
    void addToSet(SubresourceIndex index)
    {
      const auto [byteIndex, shiftIndex] = getByteAndShiftIndex(index);
      subResMaskArray[byteIndex] |= uint8_t(1) << (sub_res_available_bit_index + shiftIndex * sub_res_bits);
    }
    bool isInSet(SubresourceIndex index) const
    {
      const auto [byteIndex, shiftIndex] = getByteAndShiftIndex(index);
      return 0 != (subResMaskArray[byteIndex] & (uint8_t(1) << (sub_res_available_bit_index + shiftIndex * sub_res_bits)));
    }
    void addInlineBarrier(SubresourceIndex index)
    {
      const auto [byteIndex, shiftIndex] = getByteAndShiftIndex(index);
      subResMaskArray[byteIndex] |= uint8_t(1) << (sub_res_inline_barrier_bit_index + shiftIndex * sub_res_bits);
    }
    void removeInlineBarrier(SubresourceIndex index)
    {
      const auto [byteIndex, shiftIndex] = getByteAndShiftIndex(index);
      subResMaskArray[byteIndex] &= ~(uint8_t(1) << (sub_res_inline_barrier_bit_index + shiftIndex * sub_res_bits));
    }
    bool hasInlineBarrier(SubresourceIndex index) const
    {
      const auto [byteIndex, shiftIndex] = getByteAndShiftIndex(index);
      return 0 != (subResMaskArray[byteIndex] & (uint8_t(1) << (sub_res_inline_barrier_bit_index + shiftIndex * sub_res_bits)));
    }
    void markAsExecuted(SubresourceIndex index)
    {
      const auto [byteIndex, shiftIndex] = getByteAndShiftIndex(index);
      subResMaskArray[byteIndex] |= uint8_t(1) << (sub_res_has_executed_bit_index + shiftIndex * sub_res_bits);
    }
    bool wasExecuted(SubresourceIndex index) const
    {
      const auto [byteIndex, shiftIndex] = getByteAndShiftIndex(index);
      return 0 != (subResMaskArray[byteIndex] & (uint8_t(1) << (sub_res_has_executed_bit_index + shiftIndex * sub_res_bits)));
    }
    // has any copy pending is when any sub res has its available bit set but its executed bit is not
    bool hasAnyCopyPending() const
    {
      uint8_t availableCompundMask = 0;
      uint8_t executedCompundMask = 0;
      for (uint32_t i = 0; i < sub_res_per_byte; ++i)
      {
        availableCompundMask |= uint8_t(1) << (sub_res_available_bit_index + i * sub_res_bits);
        executedCompundMask |= uint8_t(1) << (sub_res_has_executed_bit_index + i * sub_res_bits);
      }
      for (uint32_t i = 0; i < sub_res_array_count; ++i)
      {
        auto mask = subResMaskArray[i];
        if (__popcount(mask & executedCompundMask) != __popcount(mask & availableCompundMask))
        {
          // bit count of recorded copies is not the same as executed, so at least one was not executed
          return true;
        }
      }
      return false;
    }
  };
  dag::Vector<TextureUploadRecord> uploads;
  dag::Vector<TextureSubresourceUsage> targetSubresourceBlocks;

  dag::Vector<TextureSubresourceUsage>::const_iterator getSubResIteratorInfoFor(Image *image, uint32_t sub_res_offset) const
  {
    return eastl::lower_bound(targetSubresourceBlocks.begin(), targetSubresourceBlocks.end(), 0,
      [image, sub_res_offset](const auto &l, const auto &) {
        if (l.image < image)
        {
          return true;
        }
        if (l.image > image)
        {
          return false;
        }
        return l.subResOffset < sub_res_offset;
      });
  }

  dag::Vector<TextureSubresourceUsage>::iterator getSubResIteratorInfoFor(Image *image, uint32_t sub_res_offset)
  {
    return eastl::lower_bound(targetSubresourceBlocks.begin(), targetSubresourceBlocks.end(), 0,
      [image, sub_res_offset](const auto &l, const auto &) {
        if (l.image < image)
        {
          return true;
        }
        if (l.image > image)
        {
          return false;
        }
        return l.subResOffset < sub_res_offset;
      });
  }

  const TextureSubresourceUsage *getSubResInfoFor(Image *image, uint32_t sub_res_offset) const
  {
    auto it = getSubResIteratorInfoFor(image, sub_res_offset);
    if (targetSubresourceBlocks.end() == it)
    {
      return nullptr;
    };
    if (it->image != image)
    {
      return nullptr;
    }
    if (it->subResOffset != sub_res_offset)
    {
      return nullptr;
    }
    return &*it;
  }

  TextureSubresourceUsage *getSubResInfoFor(Image *image, uint32_t sub_res_offset)
  {
    auto it = getSubResIteratorInfoFor(image, sub_res_offset);
    if (targetSubresourceBlocks.end() == it)
    {
      return nullptr;
    };
    if (it->image != image)
    {
      return nullptr;
    }
    if (it->subResOffset != sub_res_offset)
    {
      return nullptr;
    }
    return &*it;
  }

  const TextureSubresourceUsage *getSubResInfoFor(Image *image, SubresourceIndex sub_res) const
  {
    return getSubResInfoFor(image, TextureSubresourceUsage::calc_sub_res_offset(sub_res));
  }

  TextureSubresourceUsage *getSubResInfoFor(Image *image, SubresourceIndex sub_res)
  {
    return getSubResInfoFor(image, TextureSubresourceUsage::calc_sub_res_offset(sub_res));
  }

  bool hasSubRes(Image *image, SubresourceIndex sub_res) const
  {
    if (auto ptr = getSubResInfoFor(image, sub_res))
    {
      return ptr->isInSet(sub_res);
    }
    return false;
  }

  bool hasExecutedSubResCopy(Image *image, SubresourceIndex sub_res) const
  {
    if (auto ptr = getSubResInfoFor(image, sub_res))
    {
      return ptr->isInSet(sub_res) && ptr->wasExecuted(sub_res);
    }
    return false;
  }

  bool hasNoPendingSubResCopy(Image *image) const
  {
    for (auto at = getSubResIteratorInfoFor(image, 0); at != targetSubresourceBlocks.end() && at->image == image; ++at)
    {
      if (at->hasAnyCopyPending())
      {
        return false;
      }
    }
    return true;
  }

  bool hasNoPendingSubResCopy(Image *image, SubresourceIndex sub_res) const
  {
    if (auto ptr = getSubResInfoFor(image, sub_res))
    {
      return ptr->isInSet(sub_res) == ptr->wasExecuted(sub_res);
    }
    return true;
  }

  TextureSubresourceUsage *trackSubRes(Image *image, SubresourceIndex sub_res)
  {
    auto subResOffset = TextureSubresourceUsage::calc_sub_res_offset(sub_res);
    auto ref = getSubResIteratorInfoFor(image, subResOffset);
    if (targetSubresourceBlocks.end() == ref || ref->image != image || ref->subResOffset != subResOffset)
    {
      ref = targetSubresourceBlocks.insert(ref, {
                                                  .image = image,
                                                  .subResOffset = subResOffset,
                                                });
    }
    ref->addToSet(sub_res);
    return &*ref;
  }

  // walks the uploads backwards and replaces the current source when it finds a upload destination that matches
  // the source info. This tries to break copy chains, like A -> B -> C, into A -> B, A -> C, this avoids barriers
  // and allows the device to overlap the copies.
  void updateSourceInfo(TextureUploadRecord &record, uint32_t offset)
  {
    // do a fast lookup if there is anything to chain walk first
    if (!hasSubRes(record.sourceTexture, record.textureCopySourceSubresource))
    {
      return;
    }
    auto cmp = [&record](const auto &upload) { return upload.targetIsSourceOf(record); };

    // so offset is from the beginning and is the final slot we would look for from beginning to end
    // but rbegin is the start point from the end to the beginning rend. So we want the number of
    // elements from the end we want to skip
    uint32_t offsetFromEnd = uploads.size() - offset - 1;

    auto at = eastl::find_if(uploads.rbegin() + offsetFromEnd, uploads.rend(), cmp);
    if (uploads.rend() == at)
    {
      return;
    }

    if (at->isPartialCopy())
    {
      // when we see a partial copy to a parent resource, we can't break up the copy, so we stop, record that the parent has one
      // additional child.
      ++at->dependencyCount;
      record.hasParent = 1;
    }
    else
    {
      if (at->sourceTexture && hasSubRes(at->sourceTexture, at->textureCopySourceSubresource))
      {
        // this is tricky, we have now to scan for follow up copies and see if they target source, if yes, we can't skip them,
        // otherwise we may observe changes to the source, example:
        // copy A to B
        // copy D to A
        // copy B to C
        // when we shorten "B to C" into "A to C", with from front to back execution, we will observe the "D to A" copy, which
        // we should not as B would have not.
        if (at != eastl::find_if(uploads.rbegin(), at, [at](const auto &upload) { return upload.targetIsSourceOf(*at); }))
        {
          // can not shorten
          ++at->dependencyCount;
          record.hasParent = 1;
          return;
        }
      }
      // when there is no partial copy, we already reached then end as we always try to avoid copy chains from whole resources, there
      // can't be any chain other than with partial copies into the parent resource
      at->copyFromSameSource(record);
    }
  }

  void executeTextureUpload(const TextureUploadRecord &record, auto &command_list)
  {
    const auto dst = record.makeCopyDestLocation();
    const auto ofs = record.getCopyDestOffset();

    const auto src = record.makeCopySourceLocation();
    const auto box = record.getCopySourceBox();

    // makeReady is expected to make the command list read to accept commands, if it fails it is assumed there was an
    // error (device reset) and we can't execute any commands.
    if (!command_list.makeReady())
    {
      return;
    }

    if (record.sourceTexture)
    {
      auto sourceInfo = getSubResInfoFor(record.sourceTexture, record.textureCopySourceSubresource);
      if (sourceInfo && !sourceInfo->hasInlineBarrier(record.textureCopySourceSubresource))
      {
        // inlineTransition is expected to recorded needed barriers to ensure that the source texture can be copied from and an
        // upcoming copy command.
        command_list.inlineTransition(src.pResource, SubresourceIndex::make(src.SubresourceIndex));
        sourceInfo->addInlineBarrier(record.textureCopySourceSubresource);
      }
    }

    // copyResource is expected to record the copy command.
    command_list.copyResource(&dst, ofs.x, ofs.y, ofs.z, &src, box ? &*box : nullptr);

    auto dstInfo = getSubResInfoFor(record.target, record.textureCopyDestSubresource);
    G_ASSERT(dstInfo);
    if (dstInfo)
    {
      dstInfo->removeInlineBarrier(record.textureCopyDestSubresource);
      dstInfo->markAsExecuted(record.textureCopyDestSubresource);
    }

    if (!record.dependencyCount)
    {
      // exitTransition is expected to record barrier that would be needed to bring the resource an a state
      // that can be used by other command lists. See implementations for details.
      command_list.exitTransition(dst.pResource, SubresourceIndex::make(dst.SubresourceIndex));
    }
  }

  struct TextureAccessInvokeSpace
  {
    uint32_t startOffset;
    uint32_t stopOffset;
  };

  uint32_t onTextureAccessInvokeChain(TextureAccessInvokeSpace &global_search_space, TextureAccessInvokeSpace local_search_space,
    auto &&command_list, auto &&cmp)
  {
    uint32_t deletionCount = 0;
    while (local_search_space.startOffset < local_search_space.stopOffset)
    {
      auto b = uploads.begin() + local_search_space.startOffset;
      auto e = uploads.begin() + local_search_space.stopOffset;
      auto at = eastl::find_if(b, e, cmp);
      if (e == at)
      {
        break;
      }
      auto ofs = eastl::distance(uploads.begin(), at);
      if (ofs < global_search_space.startOffset)
      {
        --global_search_space.startOffset;
      }
      --global_search_space.stopOffset;
      auto copy = *at;
      uploads.erase(at);
      // +1 because we just erased one slot
      auto deletions = onTextureAccessInvokeDispatch(global_search_space, ofs, copy, command_list) + 1;
      deletionCount += deletions;
      local_search_space.startOffset -= eastl::min(local_search_space.startOffset, deletions);
      local_search_space.stopOffset -= eastl::min(local_search_space.stopOffset, deletions);
    }
    return deletionCount;
  }

  uint32_t onTextureAccessInvokeDispatch(TextureAccessInvokeSpace &global_search_space, uint32_t pivot,
    const TextureUploadRecord &record, auto &&command_list)
  {
    uint32_t deletionCount = 0;
    if (record.hasParent)
    {
      // searching for parents, can only be in the range of before and including pivot
      deletionCount += onTextureAccessInvokeChain(global_search_space,
        {.startOffset = 0, .stopOffset = eastl::min<uint32_t>(pivot + 1, uploads.size())}, command_list,
        [&record](const auto &info) { return info.targetIsSourceOf(record); });
    }
    executeTextureUpload(record, command_list);
    if (record.dependencyCount)
    {
      // searching for children, can only be in rage after pivot
      deletionCount += onTextureAccessInvokeChain(global_search_space,
        {.startOffset = pivot - eastl::min(pivot, deletionCount), .stopOffset = uploads.size()}, command_list,
        [&record](const auto &info) { return record.targetIsSourceOf(info); });
    }
    return deletionCount;
  }

  // searches in queue order if any entry matches the cmp and executes its pending commands
  uint32_t onTextureAccessInvoke(eastl::optional<uint32_t> most_recent_any_copy, auto &&command_list, auto &&cmp)
  {
    // when we know the most recent copy that effected any of the textures sub-resources we can stop when we reach one after that
    // point.
    TextureAccessInvokeSpace space{.startOffset = 0, .stopOffset = most_recent_any_copy.value_or(uploads.size() - 1) + 1};
    for (auto at = eastl::find_if(uploads.begin(), uploads.begin() + space.stopOffset, cmp); at != uploads.begin() + space.stopOffset;
         at = eastl::find_if(uploads.begin() + space.startOffset, uploads.begin() + space.stopOffset, cmp))
    {
      space.startOffset = eastl::distance(uploads.begin(), at);
      --space.stopOffset;
      auto copy = *at;
      uploads.erase(at);
      onTextureAccessInvokeDispatch(space, space.startOffset, copy, command_list);
    }
    // stop offset is one past the offset, so we need to return one before
    return space.stopOffset > 0 ? space.stopOffset - 1 : 0;
  }

public:
  void reset()
  {
    uploads.clear();
    targetSubresourceBlocks.clear();
  }

  void onTextureAccess(auto &&command_list, Image *target)
  {
    // textures with global id are never queued to upload
    if (target->hasTrackedState())
    {
      return;
    }
    if (hasNoPendingSubResCopy(target))
    {
      return;
    }
    onTextureAccessInvoke(eastl::nullopt, command_list, [target](auto &record) { return target == record.target; });
  }

  void onTextureAccess(auto &&command_list, Image *target, SubresourceIndex sub_res)
  {
    // textures with global id are never queued to upload
    if (target->hasTrackedState())
    {
      return;
    }
    if (hasNoPendingSubResCopy(target, sub_res))
    {
      return;
    }
    onTextureAccessInvoke(eastl::nullopt, command_list,
      [target, sub_res](auto &record) { return target == record.target && sub_res == record.getDestSubresourceIndex(); });
  }

  void onFlush(auto &&command_list)
  {
    for (auto &&upload : uploads)
    {
      executeTextureUpload(upload, command_list);
    }
    uploads.clear();
    targetSubresourceBlocks.clear();
  }

  class BufferToTextureCopyHandlingContext
  {
    TextureConcurrentTextureCopyManager &manager;
    Image *destination = nullptr;
    bool doConcurrentlyToFrame = false;

  public:
    BufferToTextureCopyHandlingContext(TextureConcurrentTextureCopyManager &man, Image *dst, bool disable_concurrent_execution) :
      manager{man}, destination{dst}, doConcurrentlyToFrame{!dst->hasTrackedState() && !disable_concurrent_execution}
    {}

    bool canUpload(const BufferImageCopy &region) const
    {
      if (!doConcurrentlyToFrame)
      {
        return false;
      }
      if (manager.hasExecutedSubResCopy(destination, SubresourceIndex::make(region.subresourceIndex)))
      {
        return false;
      }
      return true;
    }

    void upload(const BufferImageCopy &region, const HostDeviceSharedMemoryRegion &source)
    {
      G_ASSERT_RETURN(canUpload(region), );
      manager.uploads.push_back({
        .target = destination,
        .textureCopyDestSubresource = SubresourceIndex::make(region.subresourceIndex),
        .layout = region.layout,
        .imageOffset = region.imageOffset,
        .source = source,
      });
      manager.trackSubRes(destination, SubresourceIndex::make(region.subresourceIndex));
    }

    void trackExternalExecuted(const BufferImageCopy &region)
    {
      if (!destination->hasTrackedState())
      {
        manager.trackSubRes(destination, SubresourceIndex::make(region.subresourceIndex))
          ->markAsExecuted(SubresourceIndex::make(region.subresourceIndex));
      }
    }

    bool tryUpload(const BufferImageCopy &region, const HostDeviceSharedMemoryRegion &source)
    {
      if (canUpload(region))
      {
        upload(region, source);
        return true;
      }
      trackExternalExecuted(region);
      return false;
    }
  };

  class TextureToTextureCopyHandlingContext
  {
    TextureConcurrentTextureCopyManager &manager;
    Image *source = nullptr;
    Image *destination = nullptr;
    bool doConcurrentlyToFrame = false;
    // encodes if there is a copy at all (when it has a value) and the starting point for searching for the concrete copy
    eastl::optional<uint32_t> sourceChainHint;

    bool canUpload(SubresourceIndex source_subresource, SubresourceIndex destination_subresource) const
    {
      if (!doConcurrentlyToFrame)
      {
        return false;
      }
      if (sourceChainHint)
      {
        if (manager.hasExecutedSubResCopy(source, source_subresource))
        {
          return false;
        }
      }
      if (manager.hasExecutedSubResCopy(destination, destination_subresource))
      {
        return false;
      }
      return true;
    }

    void addUpload(SubresourceIndex source_subresource, SubresourceIndex destination_subresource) const
    {
      G_ASSERTF_RETURN(!manager.hasExecutedSubResCopy(destination, destination_subresource), ,
        "DX12: Can not queue a concurrent copy when a copy was executed on a different queue (destination)");
      G_ASSERTF_RETURN(!manager.hasExecutedSubResCopy(source, source_subresource), ,
        "DX12: Can not queue a concurrent copy when a copy was executed on a different queue (source)");
      TextureUploadRecord record{
        .target = destination,
        .sourceTexture = source,
        .textureCopyDestSubresource = destination_subresource,
        .textureCopySourceSubresource = source_subresource,
      };
      // only attempt a chain walk when we know there is a chance of finding a copy that targets the source
      if (sourceChainHint)
      {
        manager.updateSourceInfo(record, *sourceChainHint);
      }
      manager.uploads.push_back(record);
      manager.trackSubRes(destination, destination_subresource);
    }

  public:
    TextureToTextureCopyHandlingContext(TextureConcurrentTextureCopyManager &man, Image *src, Image *dst) :
      manager{man}, source{src}, destination{dst}, doConcurrentlyToFrame{!(src->hasTrackedState() || dst->hasTrackedState())}
    {
      if (!doConcurrentlyToFrame)
      {
        return;
      }

      // this will find the record block with the lowest offset
      auto at = manager.getSubResIteratorInfoFor(source, 0);
      if ((at != manager.targetSubresourceBlocks.end()) && (at->image == source))
      {
        // as its sorted, we can stop as soon we either reach the end or the image no longer matches
        for (; at != manager.targetSubresourceBlocks.end() && (at->image == source); ++at)
        {
          if (at->hasAnyCopyPending())
          {
            auto ref =
              eastl::find_if(man.uploads.rbegin(), man.uploads.rend(), [&](const auto &info) { return source == info.target; });
            G_ASSERTF(ref != man.uploads.rend(), "DX12: Expected to find any upload record, but found none");
            if (man.uploads.rend() != ref)
            {
              sourceChainHint = static_cast<uint32_t>(eastl::distance(man.uploads.begin(), &*ref));
            }
            break;
          }
        }
        if (!sourceChainHint)
        {
          // when we where not able to find any copy record, when the manager has recorded that the source was a target at some time,
          // then we know that copies where executed for this source and we can not executed concurrently to the frame
          doConcurrentlyToFrame = false;
        }
      }
    }

    // Tries to queue a copy command on the target copy manager, when it can't it will execute any copies that may be required
    // to be executed so that the source content is in the expected state. It also adds the needed tracking data to ensure consistency
    // on future copy requests. The fallback on false still have to ensure that all affected resources are transitioned into the
    // correct states before and after the fallback copy is executed.
    bool tryUpload(SubresourceIndex source_subresource, SubresourceIndex destination_subresource, auto &&command_list)
    {
      if (canUpload(source_subresource, destination_subresource))
      {
        addUpload(source_subresource, destination_subresource);
        return true;
      }
      // all this from here one out is to prepare so that the caller can execute the fallback copy on the command list of command_list
      if (sourceChainHint)
      {
        if (!manager.hasNoPendingSubResCopy(source, source_subresource))
        {
          // when there are any pending copies for the source sub-resource, we need to execute them now
          sourceChainHint = manager.onTextureAccessInvoke(sourceChainHint, command_list,
            [&](auto &record) { return source == record.target && source_subresource == record.getDestSubresourceIndex(); });
        }
      }
      if (!destination->hasTrackedState())
      {
        manager.trackSubRes(destination, destination_subresource)->markAsExecuted(destination_subresource);
      }
      return false;
    }
  };
};

struct FrameInfo
{
  CommandStreamSet<AnyCommandListPtr, D3D12_COMMAND_LIST_TYPE_DIRECT> genericCommands;
  CommandStreamSet<AnyCommandListPtr, D3D12_COMMAND_LIST_TYPE_COMPUTE> computeCommands;
  CommandStreamSet<VersionedPtr<D3DCopyCommandList>, D3D12_COMMAND_LIST_TYPE_COPY> readBackCommands;
  CommandStreamSet<AnyCommandListPtr, D3D12_COMMAND_LIST_TYPE_DIRECT> preFrameCommands;
  CommandStreamSet<VersionedPtr<D3DCopyCommandList>, D3D12_COMMAND_LIST_TYPE_COPY> frameConcurrentCommands;
  uint64_t progress = 0;
  EventPointer progressEvent{};
  dag::Vector<ProgramID> deletedPrograms;
  dag::Vector<GraphicsProgramID> deletedGraphicPrograms;
  dag::Vector<backend::ShaderModuleManager::AnyShaderModuleUniquePointer> deletedShaderModules;
  dag::Vector<Query *> deletedQueries;
  ShaderResourceViewDescriptorHeapManager resourceViewHeaps;
  SamplerDescriptorHeapManager samplerHeaps;
  BackendQueryManager backendQueryManager;
  uint32_t frameIndex = 0;

  void init(ID3D12Device *device);
  void shutdown(Device &device, DeviceQueueGroup &queue_group, PipelineManager &pipe_man);
  // returns ticks waiting for gpu
  int64_t beginFrame(DeviceQueueGroup &queue_group, PipelineManager &pipe_man, uint32_t frame_idx);
  void preRecovery(Device &device, DeviceQueueGroup &queue_group, PipelineManager &pipe_man);
  void recover(ID3D12Device *device);
};

struct SignatureStore
{
  struct SignatureInfo
  {
    ComPtr<ID3D12CommandSignature> signature;
    uint32_t stride;
    D3D12_INDIRECT_ARGUMENT_TYPE type;
  };
  struct SignatureInfoEx : SignatureInfo
  {
    ID3D12RootSignature *rootSignature;
  };

  dag::Vector<SignatureInfo> signatures;
  dag::Vector<SignatureInfoEx> signaturesEx;

  ID3D12CommandSignature *getSignatureForStride(ID3D12Device *device, uint32_t stride, D3D12_INDIRECT_ARGUMENT_TYPE type);
  ID3D12CommandSignature *getSignatureForStride(ID3D12Device *device, uint32_t stride, D3D12_INDIRECT_ARGUMENT_TYPE type,
    GraphicsPipelineSignature &signature);

  void reset()
  {
    signatures.clear();
    signaturesEx.clear();
  }
};

// NOTE: default should be values that represent ContinueCurrentFrame
struct FrameCompletionInfo
{
  enum class Mode
  {
    ContinueCurrentFrame,
    FinishCurrentFrame
  };
  Mode mode = Mode::ContinueCurrentFrame;
  uint32_t id = 0;
};
// NOTE: default should be values that represent DoNotPresent
struct PresentInfo
{
  enum class Mode
  {
    DoNotPresent,
    PresentSwapchain,
  };
  Mode mode = Mode::DoNotPresent;
  ImageViewInfo clearView = {};
  eastl::optional<uint32_t> swapchainIndex = {};
};

// need to shorten this - some Cmd structs can be combined, but this needs some refactoring in other places
class DeviceContext : protected ResourceUsageHistoryDataSetDebugger,
                      public debug::call_stack::Generator,
                      public debug::FrameCommandLogger //-V553
{
  template <typename T, typename... Args>
  T make_command(Args &&...args)
  {
    if constexpr (T::is_primary)
    {
      return T{CmdBase{}, this->generateCommandData(getRecordingFrameIndex()), eastl::forward<Args>(args)...};
    }
    else
    {
      return T{CmdBase{}, {}, eastl::forward<Args>(args)...};
    }
  }

  // Warning: intentionally not spinlock, since it has extremely bad behaviour
  // in case of high contention (e.g. during several threads of loading etc...)
  WinCritSec mutex;
  // Held only during processEmergencyDefragmentation; used by frontendSyncFence to block
  // new buffer locks while GPU memory is being moved, without coupling to the main mutex.
  WinCritSec defragGuard;
  struct WorkerThread : public DaThread
  {
    WorkerThread(DeviceContext &c) :
      DaThread("DX12 Worker", 256 << 10, cpujobs::DEFAULT_THREAD_PRIORITY + 1, WORKER_THREADS_AFFINITY_MASK), ctx(c)
    {}
    // calls device.processCommandPipe() until termination is requested
    void execute() override;
    DeviceContext &ctx;
    bool terminateIncoming = false;
  };

  // ContextState
  struct ContextState : public debug::DeviceContextState
  {
    ForwardRing<FrameInfo, FRAME_FRAME_BACKLOG_LENGTH> frames;
    SignatureStore drawIndirectSignatures;
    SignatureStore drawIndexedIndirectSignatures;
    SignatureStore dispatchRaySignatures;
    ComPtr<ID3D12CommandSignature> dispatchIndirectSignature;
    BarrierBatcher uploadBarrierBatch;
    BarrierBatcher graphicsCommandListBarrierBatch;
    BarrierBatcher pendingPreFrameBarrierBatch;
    SplitTransitionTracker graphicsCommandListSplitBarrierTracker;
    InititalResourceStateSet initialResourceStateSet;
    ResourceUsageManagerWithHistory resourceStates;
    ResourceActivationTracker resourceActivationTracker;
    BufferAccessTracker bufferAccessTracker;
    ReadBackManager readBackManager;
    TextureConcurrentTextureCopyManager textureConcurrentTextureCopyManager;
    ID3D12Resource *lastAliasBegin = nullptr;
    FramebufferLayoutManager framebufferLayouts;
    CopyCommandList<VersionedPtr<D3DCopyCommandList>> activeReadBackCommandList;
    CopyCommandList<AnyCommandListPtr> activePreFrameCommands;
    CopyCommandList<VersionedPtr<D3DCopyCommandList>> activeFrameConcurrentCommands;

    backend::BindlessSetManager bindlessSetManager;

    GraphicsState graphicsState;
    ComputeState computeState;
    PipelineStageStateBase stageState[STAGE_MAX_EXT];

    StatefulCommandBuffer cmdBuffer = {};
    uint32_t cmdBufferDebugUniqueIndex = 0;
    // it is possible that the GPU fault detection kicks in after we reused the trace recorder for the faulting frame and overwrite
    // its contents, to fix this we have two trace set we hop back and forth on each round trip.
    uint32_t cmdBufferDebugFrameHistoryPingPong = 0;
    dag::Vector<eastl::pair<size_t, size_t>> renderTargetSplitStarts;

    ActivePipeline activePipeline = ActivePipeline::Graphics;

    // on next present we should place render submit latency marker
    // because it should happen after submit yet before present
    bool needRenderSubmitLatencyMarkerBeforePresent = false;

    void nextFrame()
    {
      frames.advance();
      cmdBufferDebugUniqueIndex = 0;
      // one bit per frame index
      cmdBufferDebugFrameHistoryPingPong ^= 1u << frames.getIndex();
    }

    debug::CommandListIdentifier getCommandListDebugIdentifier() const
    {
      return debug::make_command_list_identfier(
        (frames.getIndex() << 1) | (1u & (cmdBufferDebugFrameHistoryPingPong >> frames.getIndex())), cmdBufferDebugUniqueIndex);
    }

    debug::CommandListIdentifier nextCommandListDebugIdentifier()
    {
      ++cmdBufferDebugUniqueIndex;
      return getCommandListDebugIdentifier();
    }

    void switchActivePipeline(ActivePipeline active_pipeline)
    {
      if (activePipeline == active_pipeline)
      {
        return;
      }
      // switching either to graphics or RT
      if (ActivePipeline::Graphics != activePipeline)
      {
        graphicsState.invalidateResourceStates();
        stageState[STAGE_VS].invalidateResourceStates();
        stageState[STAGE_PS].invalidateResourceStates();
      }
      // switching either to compute or RT
      if (ActivePipeline::Compute != activePipeline)
      {
        stageState[STAGE_CS].invalidateResourceStates();
      }
      activePipeline = active_pipeline;
    }

    void onFlush()
    {
      graphicsState.onFlush();
      computeState.onFlush();

      for (auto &stage : stageState)
      {
        stage.onFlush();
      }

      getFrameData().resourceViewHeaps.onFlush();
    }

    void purgeAllBindings()
    {
      for (auto &stage : stageState)
      {
        stage.resetAllState();
      }
    }

    void preRecovery(Device &device, DeviceQueueGroup &queue_group, PipelineManager &pipe_man);

    void onFrameStateInvalidate(D3D12_CPU_DESCRIPTOR_HANDLE null_ct)
    {
      graphicsState.onFrameStateInvalidate(null_ct);
      computeState.onFrameStateInvalidate();

      for (auto &stage : stageState)
      {
        stage.resetAllState();
      }

      cmdBuffer.resetForFrameStart();
      G_ASSERT(renderTargetSplitStarts.empty());
      renderTargetSplitStarts.clear();
    }

    FrameInfo &getFrameData() { return frames.get(); }
  };

  class ExecutionContext : public stackhelp::ext::ScopedCallStackContext
  {
    friend class DeviceContext;
    Device &device;
    // link to owner, needed to access device and other objects
    DeviceContext &self;
    ContextState &contextState;

#if _TARGET_SCARLETT
    ExecutionContextDataScarlett ctxScarlett;
#endif

    struct ConcurrentCopyQueueCommandListWrapper
    {
      ExecutionContext &self;

      bool makeReady() { return self.readyTextureUploadCommandList(); }
      void copyResource(const D3D12_TEXTURE_COPY_LOCATION *dst, UINT x, UINT y, UINT z, const D3D12_TEXTURE_COPY_LOCATION *src,
        const D3D12_BOX *src_box)
      {
        self.contextState.activeFrameConcurrentCommands.copyTextureRegion(dst, x, y, z, src, src_box);
      }
      // noop, auto promote / decay will move the state back to common
      void exitTransition(ID3D12Resource *, SubresourceIndex) {}
      // executes immediately the needed barrier to bring the resource from copy dst into copy source state.
      void inlineTransition(ID3D12Resource *res, SubresourceIndex sub_res)
      {
        const D3D12_RESOURCE_BARRIER barrier = {
          .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
          .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
          .Transition =
            {
              .pResource = res,
              .Subresource = sub_res.index(),
              .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
              .StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE,
            },
        };
        self.contextState.activeFrameConcurrentCommands.resourceBarrier(1, &barrier);
      }
    };

    struct BufferUploadCommandListWrapper
    {
      ExecutionContext &self;

      bool makeReady() { return self.readyBufferUploadCommandList(); }
      void copyResource(const D3D12_TEXTURE_COPY_LOCATION *dst, UINT x, UINT y, UINT z, const D3D12_TEXTURE_COPY_LOCATION *src,
        const D3D12_BOX *src_box)
      {
        self.contextState.activePreFrameCommands.copyTextureRegion(dst, x, y, z, src, src_box);
      }
      // Queues a barrier (ensures its unique) that is appended at the end of the pre frame command list before the command
      // list is closed and queued for execution. This ensures the resource is in the expected common state.
      // This transition will only be recorded when there are no consumers left at the time of recording of this barrier.
      // As this is the pre frame upload command list, after any copy was recorded to this command list, any further possible
      // reads from this res can not happen on this command list, as any future concurrent copy request would be executed
      // as inline copy command on the frame core command list, so it is safe to record this barrier.
      void exitTransition(ID3D12Resource *res, SubresourceIndex sub_res)
      {
        self.contextState.pendingPreFrameBarrierBatch.transitionUnique(res, sub_res, D3D12_RESOURCE_STATE_COPY_DEST,
          D3D12_RESOURCE_STATE_COMMON);
      }
      // executes immediately the needed barrier to bring the resource from copy dest into common state so that
      // promotion / decay rules apply again.
      void inlineTransition(ID3D12Resource *res, SubresourceIndex sub_res)
      {
        const D3D12_RESOURCE_BARRIER barrier = {
          .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
          .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
          .Transition =
            {
              .pResource = res,
              .Subresource = sub_res.index(),
              .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
              .StateAfter = D3D12_RESOURCE_STATE_COMMON,
            },
        };
        self.contextState.activePreFrameCommands.resourceBarrier(1, &barrier);
      }
    };

    struct GraphicsCommandListWrapperNoCreate
    {
      ExecutionContext &self;

      bool makeReady() { return true; }
      // in addition to executing the copy, it will also execute any outstanding barriers to ensure consistent resource states.
      void copyResource(const D3D12_TEXTURE_COPY_LOCATION *dst, UINT x, UINT y, UINT z, const D3D12_TEXTURE_COPY_LOCATION *src,
        const D3D12_BOX *src_box)
      {
        self.contextState.graphicsCommandListBarrierBatch.execute(self.contextState.cmdBuffer);
        self.contextState.cmdBuffer.copyTexture(dst, x, y, z, src, src_box);
      }
      // queues transition to common state for next barrier execution on the frame command list, so that the resource is
      // in the expected common state.
      void exitTransition(ID3D12Resource *res, SubresourceIndex sub_res)
      {
        self.contextState.graphicsCommandListBarrierBatch.transition(res, sub_res, D3D12_RESOURCE_STATE_COPY_DEST,
          D3D12_RESOURCE_STATE_COMMON);
      }
      // queues the transition from copy dest to common, the barrier will be executed by the next barrier execute command
      // to ensure consistent state.
      void inlineTransition(ID3D12Resource *res, SubresourceIndex sub_res)
      {
        self.contextState.graphicsCommandListBarrierBatch.transition(res, sub_res, D3D12_RESOURCE_STATE_COPY_DEST,
          D3D12_RESOURCE_STATE_COMMON);
      }
    };

    struct ExtendedCallStackCaptureData
    {
      DeviceContext *deviceContext;
      debug::call_stack::CommandData callStack;
      const char *lastCommandName;
      eastl::string_view lastMarker;
      eastl::string_view lastEventPath;
    };

    static constexpr uint32_t extended_call_stack_capture_data_size_in_pointers =
      (sizeof(ExtendedCallStackCaptureData) + sizeof(void *) - 1) / sizeof(void *);

    static stackhelp::ext::CallStackResolverCallbackAndSizePair on_ext_call_stack_capture(stackhelp::CallStackInfo stack,
      void *context);
    static stackhelp::ext::ResolvedRecord on_ext_call_stack_resolve(char *buf, unsigned max_buf, stackhelp::CallStackInfo stack);

    // TODO when we have different execution queues then this needs to be for a specific one
    void dirtyBufferState(BufferGlobalId ident)
    {
      contextState.graphicsState.dirtyBufferState(ident);
      for (auto &stage : contextState.stageState)
      {
        stage.dirtyBufferState(ident);
      }
    }

    // TODO when we have different execution queues then this needs to be for a specific one
    void dirtyTextureState(Image *texture)
    {
      if (!texture)
      {
        return;
      }
      if (!texture->hasTrackedState())
      {
        return;
      }
      contextState.graphicsState.dirtyTextureState(texture);
      for (auto &stage : contextState.stageState)
      {
        stage.dirtyTextureState(texture);
      }
    }

    bool readyReadBackCommandList();
    bool readyBufferUploadCommandList();
    bool readyTextureUploadCommandList();

#if _TARGET_PC_WIN
    bool readyCommandList() const { return contextState.cmdBuffer.isReadyForRecording(); }
#else
    constexpr bool readyCommandList() const { return true; }
#endif

    AnyCommandListPtr allocAndBeginCommandBuffer();
    bool checkDrawCallHasOutput(eastl::span<const char> info);
    template <size_t N>
    bool checkDrawCallHasOutput(const char (&sl)[N])
    {
      return checkDrawCallHasOutput(string_literal_span(sl));
    }

    void checkCloseCommandListResult(HRESULT result, eastl::string_view debug_name, const debug::CommandListLogger &logger) const;

    void makeSureCmdBufferHasCommands();
    void recordReadbackCommands(uint64_t progress);

  public:
    ExecutionContext(DeviceContext &ctx, ContextState &css);
    FramebufferState &getFramebufferState();
    void setUniformBuffer(uint32_t stage, uint32_t unit, const ConstBufferSetupInformationStream &info,
      StringIndexRef::RangeType name);
    void setSRVTexture(uint32_t stage, uint32_t unit, Image *image, ImageViewState view_state, bool as_donst_ds,
      D3D12_CPU_DESCRIPTOR_HANDLE view);
    void setSampler(uint32_t stage, uint32_t unit, D3D12_CPU_DESCRIPTOR_HANDLE sampler);
    void setUAVTexture(uint32_t stage, uint32_t unit, Image *image, ImageViewState view_state, D3D12_CPU_DESCRIPTOR_HANDLE view);
    void setSRVBuffer(uint32_t stage, uint32_t unit, BufferResourceReferenceAndShaderResourceView buffer);
    void setUAVBuffer(uint32_t stage, uint32_t unit, BufferResourceReferenceAndUnorderedResourceView buffer);
#if D3D_HAS_RAY_TRACING
    void setRaytraceAccelerationStructureAtT(uint32_t stage, uint32_t unit, RaytraceAccelerationStructure *as);
#endif
    void setSRVNull(uint32_t stage, uint32_t unit);
    void setUAVNull(uint32_t stage, uint32_t unit);
    void invalidateActiveGraphicsPipeline();
    void setBlendConstantFactor(E3DCOLOR constant);
    void setDepthBoundsRange(float from, float to);
    void setStencilRef(uint8_t ref);
    void setScissorEnable(bool enable);
    void setScissorRects(ScissorRectListRef::RangeType rects);
    void setIndexBuffer(BufferResourceReferenceAndAddressRange buffer, DXGI_FORMAT type);
    void setVertexBuffer(uint32_t stream, BufferResourceReferenceAndAddressRange buffer, uint32_t stride);
    void setStreamOutputBuffer(uint32_t slot, BufferResourceReferenceAndAddressRange buffer,
      BufferResourceReferenceAndAddress counter);
    bool isPartOfFramebuffer(Image *image);
    bool isPartOfFramebuffer(Image *image, MipMapRange mip_range, ArrayLayerRange array_range);
    // returns true if the current pass encapsulates the draw area of the viewport
    void updateViewports(ViewportListRef::RangeType new_vps);
    D3D12_CPU_DESCRIPTOR_HANDLE getNullColorTarget();
    void setStaticRenderState(StaticRenderStateID ident);
    void setInputLayout(InputLayoutID ident);
    void setWireFrame(bool wf);
    bool prepareCommandExecution(CommandRequirement command_requirement);
    void setConstRegisterBuffer(uint32_t stage, HostDeviceSharedMemoryRegion update);
    void writeToDebug(StringIndexRef::RangeType index);
    int64_t flush(uint64_t progress, const FrameCompletionInfo &frame_info, const PresentInfo &present_info);
    void pushEvent(StringIndexRef::RangeType name);
    void popEvent();
    void writeTimestamp(Query *query);
    void beginSurvey(PredicateInfo pi);
    void endSurvey(PredicateInfo pi);
    void beginConditionalRender(PredicateInfo pi);
    void endConditionalRender();
    void addVertexShader(ShaderID id, VertexShaderModule *sci);
    void addPixelShader(ShaderID id, PixelShaderModule *sci);
    void addComputePipeline(ProgramID id, ComputeShaderModule *csm, CSPreloaded preloaded);
    void addGraphicsPipeline(GraphicsProgramID program, ShaderID vs, ShaderID ps);
#if D3D_HAS_RAY_TRACING
    void buildBottomAccelerationStructure(uint32_t batch_size, uint32_t batch_index,
      D3D12_RAYTRACING_GEOMETRY_DESC_ListRef::RangeType geometry_descriptions,
      RaytraceGeometryDescriptionBufferResourceReferenceSetListRef::RangeType resource_refs,
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, bool update, RaytraceAccelerationStructure *dst,
      RaytraceAccelerationStructure *src, BufferResourceReferenceAndAddress scratch_buffer,
      BufferResourceReferenceAndAddress compacted_size);
#if HAS_NVAPI
    void buildBottomAccelerationStructureNvidia(RaytraceAccelerationStructure *as, RaytraceBuildFlags flags, bool update,
      BufferResourceReferenceAndAddress scratch_buffer, BufferResourceReferenceAndAddress compacted_size,
      ExtraDataArray<const NVAPI_D3D12_RAYTRACING_GEOMETRY_DESC_EX>::RangeType descs,
      RaytraceGeometryDescriptionBufferResourceReferenceSetListRef::RangeType resource_refs);
#endif
    void buildTopAccelerationStructure(uint32_t batch_size, uint32_t batch_index, uint32_t instance_count,
      BufferResourceReferenceAndAddress instance_buffer, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, bool update,
      RaytraceAccelerationStructure *dst, RaytraceAccelerationStructure *src, BufferResourceReferenceAndAddress scratch);
    void copyRaytracingAccelerationStructure(RaytraceAccelerationStructure *dst, RaytraceAccelerationStructure *src, bool compact,
      RaytraceAccelerationStructureType type);
    void buildOpacityMicroMapTriangleArrayBatchBegin(::raytrace::AccelerationStructureBuildMode mode,
      ExtraDataArray<const RayTraceOpacityMicroMapTriangleArrayBuildBufferSet>::RangeType buffer_sets);
    void buildOpacityMicroMapTriangleArrayBatchEntry(bool auto_flush, ::raytrace::AccelerationStructureBuildMode mode,
      RaytraceOpacityMicroMapTriangleArray *omm, RaytraceBuildFlags flags, BufferResourceReferenceAndAddress input_buffer,
      BufferResourceReferenceAndAddress description_buffer, uint32_t description_stride,
      BufferResourceReferenceAndAddress compacted_size_output_buffer, BufferResourceReferenceAndAddress scratch_buffer,
      ExtraDataArray<const ::raytrace::OpacityMicroMapDescription>::RangeType descs);
#endif
    void continuePipelineSetCompilation() const;
    void beginFrame(uint32_t frame_id);
    void finishFrame(uint64_t progress, Drv3dTimings *timing_data, int64_t kickoff_stamp, uint32_t front_frame, uint32_t frame_id,
      const PresentInfo &present_info);
    void dispatch(uint32_t x, uint32_t y, uint32_t z);
    void dispatchIndirect(BufferResourceReferenceAndOffset buffer);
    void copyBuffer(BufferResourceReferenceAndOffset src, BufferResourceReferenceAndOffset dst, uint32_t size);
    void updateBuffer(HostDeviceSharedMemoryRegion update, BufferResourceReferenceAndOffset dest);
    void clearBufferFloat(BufferResourceReferenceAndClearView buffer, const float values[4]);
    void clearBufferUint(BufferResourceReferenceAndClearView buffer, const uint32_t values[4]);
    void clearDepthStencilImage(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor,
      const ClearDepthStencilValue &value, D3D12_CLEAR_FLAGS depth_clear_flags, const eastl::optional<D3D12_RECT> &rect);
    void clearColorImage(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor, const ClearColorValue &value,
      const eastl::optional<D3D12_RECT> &rect);
    void copyImage(Image *src, Image *dst, const ImageCopy &copy);
    void resolveMultiSampleImage(Image *src, Image *dst);
    void blitImage(Image *src, Image *dst, ImageViewState src_view, ImageViewState dst_view,
      D3D12_CPU_DESCRIPTOR_HANDLE src_view_descroptor, D3D12_CPU_DESCRIPTOR_HANDLE dst_view_descriptor, D3D12_RECT src_rect,
      D3D12_RECT dst_rect, bool disable_predication);
    void copyQueryResult(ID3D12QueryHeap *pool, D3D12_QUERY_TYPE type, uint32_t index, uint32_t count,
      BufferResourceReferenceAndOffset buffer);
    void clearRenderTargets(ViewportState vp, uint32_t clear_mask, const E3DCOLOR *clear_color, float clear_depth,
      uint8_t clear_stencil);
    void invalidateFramebuffer();
    void flushRenderTargets();
    void flushRenderTargetStates();
    void dirtyTextureStateForFramebufferAttachmentUse(Image *texture);
    void checkFramebufferIntegrity(Image *deleted_image);
    void setGraphicsPipeline(GraphicsProgramID program);
    void setComputePipeline(ProgramID program);
    void drawIndirect(BufferResourceReferenceAndOffset buffer, uint32_t count, uint32_t stride);
    void drawIndexedIndirect(BufferResourceReferenceAndOffset buffer, uint32_t count, uint32_t stride);
    void draw(uint32_t count, uint32_t instance_count, uint32_t start, uint32_t first_instance, uint32_t num_prims_for_stats);
    void drawIndexed(uint32_t count, uint32_t instance_count, uint32_t index_start, int32_t vertex_base, uint32_t first_instance,
      uint32_t num_prims_for_stats);
    void flushViewportAndScissor();
    void flushGraphicsResourceBindings();
    void flushGraphicsMeshState();
    void flushGraphicsState(D3D12_PRIMITIVE_TOPOLOGY top);
    bool loadGraphicsPipelineVariant(D3D12_PRIMITIVE_TOPOLOGY_TYPE topType, const InputLayout &inputLayout,
      const RenderStateSystem::StaticState &staticRenderState);
    bool loadMeshPipelineVariant(const RenderStateSystem::StaticState &staticRenderState);
    bool loadComputePipeline(ComputePipeline *pipeline);
    void flushIndexBuffer();
    void flushVertexBuffers();
    void flushStreamOutputBuffer();
    void resetStreamOutputBufferOnFlush();
    void flushGraphicsStateResourceBindings();
    // handled by flush, could be moved into this though
    void ensureActivePass();
    void changePresentInterval(int interval);
#if _TARGET_XBOX
    void swapchainOnFrameBegin(FRAME_PIPELINE_TOKEN frame_token);
#endif
    void updateVertexShaderName(ShaderID shader, StringIndexRef::RangeType name);
    void updatePixelShaderName(ShaderID shader, StringIndexRef::RangeType name);
    void clearUAVTextureI(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor, const uint32_t values[4]);
    void clearUAVTextureF(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor, const float values[4]);
    void setRootConstants(unsigned stage, eastl::span<uint32_t> values);
    void registerStaticRenderState(StaticRenderStateID ident, const RenderStateSystem::StaticState &state);
    void beginVisibilityQuery(Query *q);
    void endVisibilityQuery(Query *q);
    void beginPipelineStatsQuery(PipelineStatsQuery *q, bool lazy);
    void endPipelineStatsQuery(PipelineStatsQuery *q, bool lazy);
    void cancelQuery(Query *q);
    void cancelPipelineStatsQuery(PipelineStatsQuery *q);
#if _TARGET_PC_WIN
    void changePresentWindow(uint32_t index) { self.back.swapchain.setPresentWindow(index); }
#endif
    void flushComputeState();
    void textureReadBack(Image *image, HostDeviceSharedMemoryRegion cpu_memory, BufferImageCopyListRef::RangeType regions);
    void bufferReadBack(BufferResourceReferenceAndRange buffer, HostDeviceSharedMemoryRegion cpu_memory, size_t offset);
#if !_TARGET_XBOXONE
    void setVariableRateShading(D3D12_SHADING_RATE rate, D3D12_SHADING_RATE_COMBINER vs_combiner,
      D3D12_SHADING_RATE_COMBINER ps_combiner);
    void setVariableRateShadingTexture(Image *texture);
#endif
    void registerInputLayout(InputLayoutID ident, const InputLayout &layout);
    void createDlssFeature(bool stereo_render, int output_width, int output_height);
    void createDlssFeature(int mode, int output_width, int output_height, bool use_rr, bool use_legacy_model);
    void releaseDlssFeature(bool stereo_render);
    void executeDlss(const nv::DlssParams<Image> &dlss_params, int view_index);
    void executeDlssG(const nv::DlssGParams<Image> &dlss_g_params, int view_index);
    void setDlssGEnabled(int frames_to_generate, int view_index);
    void setDlssOptions(const nv::DlssOptions &options, int view_index);
    void prepareExecuteAA(std::initializer_list<Image *> inputs, std::initializer_list<Image *> outputs);
    void executeXess(const XessParamsDx12 &params);
    void executeXeFg(const XessFgParamsDx12 &params);
    void executeFSR(const FSRUpscalingArgs &params);
    void executeFSRFG(const FSRFrameGenArgs &params);
    void removeVertexShader(ShaderID shader);
    void removePixelShader(ShaderID shader);
    void deleteProgram(ProgramID program);
    void deleteGraphicsProgram(GraphicsProgramID program);
    void deleteQueries(QueryPointerListRef::RangeType queries);
    void hostToDeviceMemoryCopy(BufferResourceReferenceAndRange target, HostDeviceSharedMemoryRegion source, size_t source_offset);
    void initializeTextureState(D3D12_RESOURCE_STATES state, ValueRange<ExtendedImageGlobalSubresourceId> id_range);
    void uploadTexture(Image *target, BufferImageCopyListRef::RangeType regions, HostDeviceSharedMemoryRegion source,
      DeviceQueueType queue, bool is_discard);
    void beginCapture(UINT flags, WStringIndexRef::RangeType name);
    void endCapture();
    void captureNextFrames(UINT flags, WStringIndexRef::RangeType name, int frame_count);
#if _TARGET_XBOX
    void updateFrameInterval(int32_t freq_level);
    void resummarizeHtile(ID3D12Resource *depth);
#endif
    void bufferBarrier(BufferResourceReference buffer, ResourceBarrier barrier, GpuPipeline queue);
    void textureBarrier(Image *tex, SubresourceRange sub_res_range, uint32_t tex_flags, ResourceBarrier barrier, GpuPipeline queue,
      bool force_barrier);
    void enhancedTextureBarrier(const d3d::TextureBarrier &barrier, Image *image);
    void enhancedBufferBarrier(const d3d::BufferBarrier &barrier, BufferResourceReference buffer);
    void discardTexture(Image *tex, uint32_t tex_flags);
#if D3D_HAS_RAY_TRACING
    void asBarrier(RaytraceAccelerationStructure *as, GpuPipeline queue);
#endif
    void terminateWorker() { self.worker->terminateIncoming = true; }

#if _TARGET_XBOX
    void enterSuspendState();
#endif
    void writeDebugMessage(StringIndexRef::RangeType message, int severity);

    void bindlessSetResourceDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
    void deferBindlessSetResourceDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
    void bindlessSetSamplerDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

    void registerFrameCompleteEvent(os_event_t event);

    void registerFrameEventsCallback(FrameEvents *callback);
    void onBeginCPUTextureAccess(Image *image);
    void onEndCPUTextureAccess(Image *image);

    void addSwapchainView(Image *image, ImageViewInfo view);
    void mipMapGenSource(Image *image, MipMapIndex mip, ArrayLayerIndex ary);
    void disablePredication();
    void tranistionPredicationBuffer();
    void applyPredicationBuffer();
    void setLatencyMarker(uint32_t frame_id, lowlatency::LatencyMarkerType type);

#if _TARGET_PC_WIN
    void onDeviceError(HRESULT remove_reason);
    void commandFence(std::atomic_uint32_t &signal);
#endif

#if _TARGET_PC_WIN
    void beginTileMapping(Image *image, ID3D12Heap *heap, size_t heap_base, size_t mapping_count);
#else
    void beginTileMapping(Image *image, uintptr_t address, uint64_t size, size_t mapping_count);
#endif
    void addTileMappings(const TileMapping *mapping, size_t mapping_count);
    void endTileMapping();

    void activateBuffer(BufferResourceReferenceAndAddressRangeWithClearView buffer, const ResourceMemoryLocation &memory_location,
      ResourceActivationAction action, GpuPipeline gpu_pipeline);
    void activateTexture(Image *tex, ResourceActivationAction action, const ResourceClearValue &value, ImageViewState view_state,
      D3D12_CPU_DESCRIPTOR_HANDLE view, GpuPipeline gpu_pipeline);
    void deactivateBuffer(BufferResourceReferenceAndAddressRange buffer, const ResourceMemoryLocation &memory_location,
      GpuPipeline gpu_pipeline);
    void deactivateTexture(Image *tex, GpuPipeline gpu_pipeline);
    void aliasFlush(GpuPipeline gpu_pipeline);
    void twoPhaseCopyBuffer(BufferResourceReferenceAndOffset source, uint64_t destination_offset, ScratchBuffer scratch_memory,
      uint64_t data_size);
    void moveBuffer(BufferResourceReferenceAndRange from, BufferResourceReferenceAndOffset to);
    void twoPhaseMoveBuffer(BufferResourceReferenceAndRange from, BufferResourceReferenceAndOffset to, ScratchBuffer scratch_memory);
    void moveTexture(Image *from, Image *to);
    void twoPhaseMoveTexture(Image *from, Image *to, ScratchBuffer scratch_memory);

    void transitionBuffer(BufferResourceReference buffer, D3D12_RESOURCE_STATES state);

    void resizeImageMipMapTransfer(Image *src, Image *dst, MipMapRange mip_map_range, uint32_t src_mip_map_offset,
      uint32_t dst_mip_map_offset);

    void debugBreak();
    void addDebugBreakString(StringIndexRef::RangeType str);
    void removeDebugBreakString(StringIndexRef::RangeType str);

#if !_TARGET_XBOXONE
    void dispatchMesh(uint32_t x, uint32_t y, uint32_t z);
    void dispatchMeshIndirect(BufferResourceReferenceAndOffset args, uint32_t stride, BufferResourceReferenceAndOffset count,
      uint32_t max_count);
#endif
    void addShaderGroup(uint32_t group, ScriptedShadersBinDumpOwner *dump, ShaderID null_pixel_shader, StringIndexRef::RangeType name);
    void removeShaderGroup(uint32_t group);
    void loadComputeShaderFromDump(ProgramID program);

    static bool should_pipeline_set_compilation_spread_over_frames();
    static bool should_use_pipeline_set_compile_worker();
    void compilePipelineSet(DynamicArray<InputLayoutIDWithHash> &&input_layouts,
      DynamicArray<StaticRenderStateIDWithHash> &&static_render_states, DynamicArray<FramebufferLayoutWithHash> &&framebuffer_layouts,
      DynamicArray<cacheBlk::SignatureEntry> &&scripted_shader_dump_signature,
      DynamicArray<cacheBlk::ComputeClassUse> &&compute_pipelines, DynamicArray<cacheBlk::GraphicsVariantGroup> &&graphics_pipelines,
      DynamicArray<cacheBlk::GraphicsVariantGroup> &&graphics_with_null_override_pipelines, ShaderID null_pixel_shader);

#if _TARGET_PC_WIN
    void setAsyncPsoCompilationMode(PipelineManager::AsyncPsoMode mode);
#endif

    void switchActivePipeline(ActivePipeline pipeline);

#if D3D_HAS_RAY_TRACING
    void applyRaytraceState(const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt,
      UInt32ListRef::RangeType root_constants);
    void dispatchRays(const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt,
      UInt32ListRef::RangeType root_constants, const RayDispatchParameters &rdp);
    void dispatchRaysIndirect(const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt,
      UInt32ListRef::RangeType root_constants, const RayDispatchIndirectParameters &rdip);
    void accelerationStructurePoolBarrier(::raytrace::AccelerationStructurePool pool);
#endif
    void executeFaultyTextureRead(D3D12_CPU_DESCRIPTOR_HANDLE src_descriptor, Image *dst_image, ImageViewState dst_view,
      D3D12_CPU_DESCRIPTOR_HANDLE dst_descriptor);

    void switchSyncMode(bool postpone);

    void setGpuPostmortemDataTraceEnabled(bool is_enabled);

    void profileMarker(da_profiler::desc_id_t marker) { self.profilerStack.pushChained(marker); }
    void popProfileMarker() { self.profilerStack.popChained(); }

#if D3D_HAS_RAY_TRACING
    void setComputeOnRayTraceShaderBindingTableConstBuffer(uint32_t stage, uint32_t slot, ProgramID program);
#endif

  private:
    static void validate_globals_size(const dxil::ShaderHeader &header, const PipelineStageStateBase &stage_state, ShaderStage stage,
      ID3D12PipelineState *pipeline);
  };

  struct FrontendFrameLatchedData
  {
    uint64_t progress = 0;
#if DX12_RECORD_TIMING_DATA
    uint32_t frameIndex = 0;
#endif
    dag::Vector<Query *> deletedQueries;
    dag::Vector<PipelineStatsQuery *> deletedPipelineStatsQueries;
  };

  struct Frontend
  {
    eastl::array<FrontendFrameLatchedData, FRAME_FRAME_BACKLOG_LENGTH> latchedFrameSet = {};
    FrontendFrameLatchedData *recordingLatchedFrame = nullptr;
    uint32_t activeRangedQueries = 0;
    uint32_t frameIndex = 0;
    uint64_t nextWorkItemProgress = 2;
    uint64_t recordingWorkItemProgress = 1;
    uint64_t completedFrameProgress = 0;
    // video/cpuGpuOverlap:b=off forces the frontend to block until the GPU finished the submitted frame
    bool disableCpuGpuOverlap = false;
    dag::Vector<FrameEvents *> frameEventCallbacks;
    frontend::Swapchain swapchain;
#if DX12_RECORD_TIMING_DATA
    Drv3dTimings timingHistory[timing_history_length]{};
    uint32_t completedFrameIndex = 0;
    int64_t lastPresentTimeStamp = 0;
#if DX12_CAPTURE_AFTER_LONG_FRAMES
    struct
    {
      int64_t thresholdUS = -1;
      int64_t captureId = 0;
      UINT flags = 0x10000 /* D3D12XBOX_PIX_CAPTURE_API */;
      int frameCount = 1;
      int captureCountLimit = -1;
      int ignoreNextFrames = 5;
    } captureAfterLongFrames;
#endif
#endif
#if _TARGET_PC
    bool isGpuLatencyEnabled = true;
    OSSpinlock latencyWaitGuard;
    alignas(std::hardware_constructive_interference_size) std::atomic_uint32_t waitForSwapchainCount = 0;
#endif
  };
  struct Backend
  {
    // state used by any context
    ContextState sharedContextState;
    dag::Vector<os_event_t> frameCompleteEvents;
    dag::Vector<FrameEvents *> frameEventCallbacks;
#if DX12_RECORD_TIMING_DATA
    int64_t gpuWaitDuration = 0;
    int64_t acquireBackBufferDuration = 0;
    int64_t workWaitDuration = 0;
#endif
    int64_t previousPresentEndTicks = 0;
    backend::Swapchain swapchain{};
    // The backend has its own frame progress counter, as its simpler than syncing
    // it with the front end one.
    uint64_t frameProgress = 0;
  };
  friend class Device;
  friend class frontend::Swapchain;
  friend class backend::Swapchain;
  Device &device;

  SyncedCommandStore commandStream;
  eastl::unique_ptr<WorkerThread> worker;
#if !DX12_FIXED_EXECUTION_MODE
  CommandExecutionMode executionMode = CommandExecutionMode::IMMEDIATE;
  bool isImmediateFlushSuppressed = false;
#endif
  WIN_MEMBER std::atomic_bool isWaitForAsyncPresent = false;
#if ENABLE_GENERIC_RENDER_PASS_VALIDATION
  eastl::optional<RenderPassArea> activeRenderPassArea;
#endif

  Frontend front;
  Backend back = {};

#if _TARGET_XBOX
  EventPointer enteredSuspendedStateEvent;
  EventPointer resumeExecutionEvent;
#endif

  EventsPool eventsPool;

  XessWrapper xessWrapper;
  FsrWrapper fsrWrapper;
#if _TARGET_PC_WIN
  eastl::optional<StreamlineAdapter> streamlineAdapter;
#endif

#if USE_DLSS_WITHOUT_STREAMLINE
  DLSSSuperResolutionDirect dlssInterface;
#endif

  StackedProfileEvents profilerStack;
  uint32_t minPipelinesToCompilePerFrame = 10;

#if _TARGET_XBOX
  bool xboxMainThreadFramePacing = true;
#endif

  alignas(std::hardware_constructive_interference_size) std::atomic_uint32_t presentedFrameId = 0;

#if DX12_FIXED_EXECUTION_MODE
  constexpr bool isImmediateMode() const { return false; }
  constexpr bool isImmediateFlushMode() const { return false; }
  constexpr void immediateModeExecute(bool = false) const {}
  constexpr void suppressImmediateFlush() {}
  constexpr void restoreImmediateFlush() {}
#else
  bool isImmediateMode() const
  {
    return CommandExecutionMode::CONCURRENT != executionMode && CommandExecutionMode::CONCURRENT_SYNCED != executionMode;
  }
  bool isImmediateFlushMode() const { return CommandExecutionMode::IMMEDIATE_FLUSH == executionMode; }
  void suppressImmediateFlush()
  {
    if (executionMode == CommandExecutionMode::IMMEDIATE_FLUSH)
    {
      setExecutionMode(CommandExecutionMode::IMMEDIATE);
      isImmediateFlushSuppressed = true;
    }
  }
  void restoreImmediateFlush()
  {
    if (isImmediateFlushSuppressed)
    {
      isImmediateFlushSuppressed = false;
      setExecutionMode(CommandExecutionMode::IMMEDIATE_FLUSH);
    }
  }

  // if immediate mode is enabled then this executes all queued commands in front.commandStream and
  // if immediate flush mode is enabled and the flush parameter is true then current thread will wait
  // for the GPU to finish the commands clears the stream after the execution
  void immediateModeExecute(bool flush = false);

  void setExecutionMode(CommandExecutionMode execution_mode)
  {
    executionMode = execution_mode;
    commandStream.setExecutionMode(executionMode);
  }
#endif

  bool replayCommands(ExecutionContext &execution_context);
  void replayCommandsConcurrently(volatile int &terminate);

  enum class TidyFrameMode
  {
    FrameCompleted,
    SyncPoint,
  };
  void frontFlush(TidyFrameMode tidy_mode);
  void flushCommandsAndFrontend();
  bool waitForLatchedFrame();
  void manageLatchedState(TidyFrameMode tidy_mode);

  void makeReadyForFrame(uint32_t frame_index, bool update_swapchain = true);
  void initFrameStates();
  void shutdownFrameStates();
  WinCritSec &getFrontGuard();
#if DX12_FIXED_EXECUTION_MODE
  void initMode();
#else
  void initMode(CommandExecutionMode mode);
#endif
  void shutdownWorkerThread();

  void discardTextureInternal(Image *image, uint32_t tex_flags);

  // assumes frontend and backend are synced and locked so that access to front and back is safe
  void resizeSwapchain(Extent2D size, uint32_t swapchain_index);
  void waitInternal();
  void finishInternal();
  void blitImageInternal(Image *src, Image *dst, const ImageBlit &region, bool disable_predication);
  void tidyFrame(FrontendFrameLatchedData &frame, TidyFrameMode mode);
#if _TARGET_PC_WIN
  void onDeviceError(HRESULT remove_reason);
  // Blocks the execution until all previously added commands will be executed at backend
  void waitForCommandFence();
#endif

public:
  DeviceContext() = delete;
  ~DeviceContext() = default;

  WinCritSec &getDefragGuard();

  DeviceContext(const DeviceContext &) = delete;
  DeviceContext &operator=(const DeviceContext &) = delete;

  DeviceContext(DeviceContext &&) = delete;
  DeviceContext &operator=(DeviceContext &&) = delete;

  DeviceContext(Device &dvc) : device(dvc) {}

  void clearRenderTargets(ViewportState vp, uint32_t clear_mask, const E3DCOLOR *clear_color, float clear_depth,
    uint8_t clear_stencil);

  void pushConstRegisterData(uint32_t stage, eastl::span<const ConstRegisterType> data);

  void setSRVTexture(uint32_t stage, size_t unit, BaseTex *texture, ImageViewState view, bool as_const_ds);
  void setSampler(uint32_t stage, size_t unit, D3D12_CPU_DESCRIPTOR_HANDLE sampler);
  void setSamplerHandle(uint32_t stage, size_t unit, d3d::SamplerHandle sampler);
  void setUAVTexture(uint32_t stage, size_t unit, Image *image, ImageViewState view_state);

  void setSRVBuffer(uint32_t stage, size_t unit, BufferResourceReferenceAndShaderResourceView buffer);
  void setUAVBuffer(uint32_t stage, size_t unit, BufferResourceReferenceAndUnorderedResourceView buffer);
  void setConstBuffer(uint32_t stage, size_t unit, const ConstBufferSetupInformationStream &info, const char *name);

  void setSRVNull(uint32_t stage, uint32_t unit);
  void setUAVNull(uint32_t stage, uint32_t unit);

  void setBlendConstant(E3DCOLOR color);
  void setDepthBoundsRange(float from, float to);
  void setPolygonLine(bool enable);
  void setStencilRef(uint8_t ref);
  void setScissorEnable(bool enabled);
  void setScissorRects(dag::ConstSpan<D3D12_RECT> rects);

#if _TARGET_PC_WIN
  void flushAndPresentToWindow(HWND hwnd);
#endif

  void bindVertexDecl(InputLayoutID ident);

  void setIndexBuffer(BufferResourceReferenceAndAddressRange buffer, DXGI_FORMAT type);

  void bindVertexBuffer(uint32_t stream, BufferResourceReferenceAndAddressRange buffer, uint32_t stride);

  void setStreamOutputBuffer(uint32_t slot, BufferResourceReferenceAndAddressRange buffer, BufferResourceReferenceAndAddress counter);

  void dispatch(uint32_t x, uint32_t y, uint32_t z);
  void dispatchIndirect(BufferResourceReferenceAndOffset buffer);
  void drawIndirect(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t count, BufferResourceReferenceAndOffset buffer, uint32_t stride);
  void drawIndexedIndirect(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t count, BufferResourceReferenceAndOffset buffer, uint32_t stride);
  void draw(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t start, uint32_t count, uint32_t first_instance, uint32_t instance_count,
    uint32_t num_prims_for_stats);
  void drawIndexed(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t index_start, uint32_t count, int32_t vertex_base, uint32_t first_instance,
    uint32_t instance_count, uint32_t num_prims_for_stats);
  void setComputePipeline(ProgramID program);
  void setGraphicsPipeline(GraphicsProgramID program);
  void copyBuffer(BufferResourceReferenceAndOffset source, BufferResourceReferenceAndOffset dest, uint32_t data_size);
  void updateBufferNoLock(HostDeviceSharedMemoryRegion update, BufferResourceReferenceAndOffset dest);
  void clearBufferFloat(BufferResourceReferenceAndClearView buffer, const float values[4]);
  void clearBufferInt(BufferResourceReferenceAndClearView buffer, const unsigned values[4]);
  void pushEvent(const char *name);
  void popEvent();
  void updateViewports(dag::ConstSpan<ViewportState> viewports);
  void clearDepthStencilImage(Image *image, const ImageSubresourceRange &area, const ClearDepthStencilValue &value,
    D3D12_CLEAR_FLAGS depth_clear_flags, const eastl::optional<D3D12_RECT> &rect);
  void clearColorImage(Image *image, const ImageSubresourceRange &area, const ClearColorValue &value,
    const eastl::optional<D3D12_RECT> &rect);
  void copyImage(Image *src, Image *dst, const ImageCopy &copy);
  void blitImage(Image *src, Image *dst, const ImageBlit &region);
  void resolveMultiSampleImage(Image *src, Image *dst);
  void flushDraws();
  void flushDrawsNoLock();
  bool noActiveQueriesNoLock();
  void wait();
  void beginSurvey(int name);
  void endSurvey(int name);
  void destroyBuffer(BufferState buffer);
  void discardBuffer(BufferState &to_discared_ref, DeviceMemoryClass memory_class, FormatStore format, uint32_t struct_size,
    bool raw_view, bool struct_view, D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name);
  void checkFramebufferIntegityNoLock(Image *img);
  void destroyImageNoLock(Image *img, bool is_rt);
  // blocks the input sampling until the GPU finished the running frame
  void gpuLatencyWait();
  void beginFrame(uint32_t frame_id, bool allow_wait);
  void setLatencyMarker(uint32_t frame_id, lowlatency::LatencyMarkerType type);
  void finishFrame(uint32_t frame_id, bool present_on_swapchain = true);
  void disableCpuGpuOverlap() { front.disableCpuGpuOverlap = true; }
  void changePresentInterval(int interval);
  int getPresentInterval();
  void changeCurrentSwapchainExtents(Extent2D size, bool should_change_hdr);
#if _TARGET_PC_WIN
  void changePresentWindowNoLock(uint32_t index);
  void changeSwapchainExtents(Extent2D size, uint32_t swapchian_index);
  HRESULT getSwapchainDesc(DXGI_SWAP_CHAIN_DESC *out_desc) const;
  IDXGIOutput *getSwapchainOutput() const;
#endif
  void shutdownSwapchain();
  void insertTimestampQuery(Query *query);
  void deleteQuery(Query *query);
  void deletePipelineStatsQuery(PipelineStatsQuery *query);
  void generateMipmaps(Image *img);
  void setFramebuffer(Image **image_list, ImageViewState *view_list, bool read_only_depth);
#if D3D_HAS_RAY_TRACING
  void raytraceBuildBottomAccelerationStructure(uint32_t batch_size, uint32_t batch_index, RaytraceBottomAccelerationStructure *as,
    const RaytraceGeometryDescription *descs, uint32_t count, RaytraceBuildFlags flags, bool update, Sbuffer *scratch_buffer,
    uint32_t scratch_buffer_offset, Sbuffer *compacted_size_buffer, uint32_t compacted_size_offset);
  void raytraceBuildTopAccelerationStructure(uint32_t batch_size, uint32_t batch_index, RaytraceTopAccelerationStructure *as,
    Sbuffer *instance_buffer, uint32_t instance_count, RaytraceBuildFlags flags, bool update, Sbuffer *scratch_buffer,
    uint32_t scratch_buffer_offset);
  void buildOpacityMicroMapTriangleArray(eastl::span<const ::raytrace::BatchedOpacityMicroMapTriangleArrayBuildInfo> builds,
    bool auto_flush, ::raytrace::AccelerationStructureBuildMode mode);
  void raytraceCopyAccelerationStructure(RaytraceAccelerationStructure *dst, RaytraceAccelerationStructure *src, bool compact,
    RaytraceAccelerationStructureType type);
  void deleteRaytraceBottomAccelerationStructure(RaytraceBottomAccelerationStructure *desc);
  void deleteRaytraceTopAccelerationStructure(RaytraceTopAccelerationStructure *desc);
  void deleteRaytraceOpacityMicroMapTriangleArray(RaytraceOpacityMicroMapTriangleArray *desc);
  void setRaytraceAccelerationStructure(uint32_t stage, size_t unit, RaytraceAccelerationStructure *as);
#endif
  void beginConditionalRender(int name);
  void endConditionalRender();

  void addVertexShader(ShaderID id, eastl::unique_ptr<VertexShaderModule> shader);
  void addPixelShader(ShaderID id, eastl::unique_ptr<PixelShaderModule> shader);
  void removeVertexShader(ShaderID id);
  void removePixelShader(ShaderID id);

  void addGraphicsProgram(GraphicsProgramID program, ShaderID vs, ShaderID ps);
  void addComputeProgram(ProgramID id, eastl::unique_ptr<ComputeShaderModule> csm, CSPreloaded preloaded);
  void removeProgram(ProgramID program);

  void placeAftermathMarker(const char *name);
  void updateVertexShaderName(ShaderID shader, const char *name);
  void updatePixelShaderName(ShaderID shader, const char *name);
  void setImageResourceState(D3D12_RESOURCE_STATES state, ValueRange<ExtendedImageGlobalSubresourceId> range);
  void setImageResourceStateNoLock(D3D12_RESOURCE_STATES state, ValueRange<ExtendedImageGlobalSubresourceId> range);
  void clearUAVTexture(Image *image, ImageViewState view, const unsigned values[4]);
  void clearUAVTexture(Image *image, ImageViewState view, const float values[4]);
  void setRootConstants(unsigned stage, eastl::span<const uint32_t> values);
  void beginGenericRenderPassChecks(const RenderPassArea &renderPassArea);
  void endGenericRenderPassChecks();
#if _TARGET_PC_WIN
  void preRecovery(bool is_device_lost);
  void recover(const dag::Vector<D3D12_CPU_DESCRIPTOR_HANDLE> &unbounded_samplers);
#endif
  void deleteTexture(BaseTex *tex);
  void resetBindlessReferences(BaseTex *tex);
  void resetBindlessReferences(BufferState &buffer);
  bool updateBindlessReferences(D3D12_CPU_DESCRIPTOR_HANDLE old_descriptor, D3D12_CPU_DESCRIPTOR_HANDLE new_descriptor);
  void freeMemory(HostDeviceSharedMemoryRegion allocation);
  void freeMemoryOfUploadBuffer(HostDeviceSharedMemoryRegion allocation);
  void uploadToBufferNoLock(BufferResourceReferenceAndRange target, HostDeviceSharedMemoryRegion memory, size_t m_offset);
  void readBackFromBufferNoLock(HostDeviceSharedMemoryRegion memory, size_t m_offset, BufferResourceReferenceAndRange source);
  void uploadToImage(const BaseTex &dst_tex, const BufferImageCopy *regions, uint32_t region_count,
    HostDeviceSharedMemoryRegion memory, DeviceQueueType queue, bool is_discard);
  // Return value is the progress the caller can wait on to ensure completion of this operation
  uint64_t readBackFromImage(HostDeviceSharedMemoryRegion memory, const BufferImageCopy *regions, uint32_t region_count,
    Image *source);
  void removeGraphicsProgram(GraphicsProgramID program);
  void registerStaticRenderState(StaticRenderStateID ident, const RenderStateSystem::StaticState &state);
  void setStaticRenderState(StaticRenderStateID ident);
  void beginVisibilityQuery(Query *q);
  void endVisibilityQuery(Query *q);
  void beginPipelineStatsQuery(PipelineStatsQuery *q, bool lazy);
  void endPipelineStatsQuery(PipelineStatsQuery *q, bool lazy);
#if _TARGET_XBOX
  // Protocol for suspend:
  // 1) Locks context
  // 2) Signals suspend event
  // 3) Wait for suspend executed event
  // 4) Suspend DX12 API
  void suspendExecution();
  // Protocol for resume:
  // 1) Resume DX12 API
  // 2) Restore Swapchain
  // 3) Signal resume event
  // 4) Unlock context
  void resumeExecution();

  void updateFrameInterval(int32_t freq_level = -1);
  void resummarizeHtile(BaseTex *bt);
#endif
  // Returns the progress we are recording now for future push to the GPU
  uint64_t getRecordingFenceProgress() const { return front.recordingWorkItemProgress; }
  // Returns current progress of the GPU
  uint64_t getCompletedFenceProgress() const { return front.completedFrameProgress; }
  uint32_t getRecordingFrameIndex() const { return front.frameIndex; }
  void waitForProgress(uint64_t progress);
  void beginStateCommit() { mutex.lock(); }
  void endStateCommit() { mutex.unlock(); }
#if !_TARGET_XBOXONE
  void setVariableRateShading(D3D12_SHADING_RATE rate, D3D12_SHADING_RATE_COMBINER vs_combiner,
    D3D12_SHADING_RATE_COMBINER ps_combiner);
  void setVariableRateShadingTexture(Image *texture);
#endif
  void registerInputLayout(InputLayoutID ident, const InputLayout &layout);
  void initStreamline(DXGIAdapter *adapter);
  void initXeSS();
  void initFSR();
  void initDLSS();
  void shutdownStreamline();
  void shutdownXess();
  void shutdownFSR();
  void shutdownDLSS();
  XessState getXessState() const { return xessWrapper.getXessState(); }

  bool isXeFGSupported() const { return xessWrapper.isFrameGenerationSupported(); }
  bool isXeFGEnabled() const { return xessWrapper.isFrameGenerationEnabled(); }
  void enableXeFG(bool enable) { xessWrapper.enableFrameGeneration(enable); }
  void suppressXeFG(bool suppress) { xessWrapper.suppressFrameGeneration(suppress); }
  void scheduleXeFG(const XessFgParams &params);
  int getXeFgPresentedFrameCount() { return xessWrapper.getPresentedFrameCount(); }

  bool isFsrLoaded() const { return fsrWrapper.isLoaded(); }
  bool isFsrUpscalingSupported() const { return fsrWrapper.isUpscalingSupported(); }
  bool initFsrUpscaling(const amd::FSR::ContextArgs &args) { return fsrWrapper.fsrCreateFeature(args); }
  void teardownFsrUpscaling() { fsrWrapper.teardownUpscaling(); }
  bool isFsrFGSupported() const { return fsrWrapper.isFrameGenerationSupported(); }
  bool isFsrFGEnabled() const { return fsrWrapper.isFrameGenerationEnabled(); }
  bool isFsrFGSuppressed() const { return fsrWrapper.isFrameGenerationSuppressed(); }
  void enableFsrFG(bool enable) { fsrWrapper.enableFrameGeneration(enable); }
  void suppressFsrFG(bool suppress) { fsrWrapper.suppressFrameGeneration(suppress); }
  int getFsrFgPresentedFrameCount() { return fsrWrapper.getPresentedFrameCount(); }

  void shutdownInternalSwapchain();
  bool adoptUserSwapchain(DXGISwapChain *swapchain, SwapchainCreateInfo &&sci);
  bool createDefaultSwapchain(DXGIFactory *factory, SwapchainCreateInfo &&sci);

  void preRecoverStreamline();
  void recoverStreamline(DXGIAdapter *adapter);
#if _TARGET_PC_WIN
  eastl::optional<StreamlineAdapter> &getStreamlineAdapter() { return streamlineAdapter; }
#endif
#if USE_DLSS_WITHOUT_STREAMLINE
  nv::DLSS &getDlss() { return dlssInterface; }
#endif
  bool isXessQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int xess_quality) const
  {
    return xessWrapper.isXessQualityAvailableAtResolution(target_width, target_height, xess_quality);
  }
  void getXessRenderResolution(int &w, int &h, int &minw, int &minh, int &maxw, int &maxh) const
  {
    xessWrapper.getXeSSRenderResolution(w, h, minw, minh, maxw, maxh);
  }
  IPoint2 getFsrRenderResolution(amd::FSR::UpscalingMode mode, const IPoint2 &target_resolution) const
  {
    return fsrWrapper.getFsrRenderResolution(mode, target_resolution);
  }
  dag::Expected<eastl::string, XessWrapper::ErrorKind> getXessVersion() const { return xessWrapper.getVersion(); }
  String getFsrVersion() const { return fsrWrapper.getVersion(); }
  void setXessVelocityScale(float x, float y) { xessWrapper.setVelocityScale(x, y); }
  void createDlssFeature(bool stereo_render, int output_width, int output_height);
  void createDlssFeature(int mode, int output_width, int output_height, bool use_rr, bool use_legacy_dlss);
  void releaseDlssFeature(bool stereo_render);
  void executeDlss(const nv::DlssParams<BaseTexture> &dlss_params, int view_index);
  void executeDlssG(const nv::DlssGParams<BaseTexture> &dlss_g_params, int view_index);
  void setDlssGEnabled(int frames_to_generate, int view_index);
  void setDlssOptions(const nv::DlssOptions &options, int view_index);
  void executeXess(const XessParams &params);
  void executeFSR(const amd::FSR::UpscalingArgs &params);
  void executeFSRFG(const amd::FSR::FrameGenArgs &params);
  void bufferBarrier(BufferResourceReference buffer, ResourceBarrier barrier, GpuPipeline queue);
  void textureBarrier(Image *tex, SubresourceRange sub_res_range, uint32_t tex_flags, ResourceBarrier barrier, GpuPipeline queue,
    bool force_barrier);
  void enhancedTextureBarrier(const d3d::TextureBarrier &barrier, BaseTexture *texture);
  void enhancedBufferBarrier(const d3d::BufferBarrier &barrier, Sbuffer *buffer);
  void discardTexture(BaseTex *tex);
#if D3D_HAS_RAY_TRACING
  void blasBarrier(RaytraceAccelerationStructure *as, GpuPipeline queue);
#endif
  void beginCapture(UINT flags, LPCWSTR name);
  void endCapture();
  void captureNextFrames(UINT flags, LPCWSTR name, int frame_count);
  void writeDebugMessage(const char *msg, intptr_t msg_length, intptr_t severity);
#if DX12_RECORD_TIMING_DATA
  const Drv3dTimings &getTiming(uintptr_t offset) const
  {
    return front.timingHistory[(front.completedFrameIndex - offset) % timing_history_length];
  }
#if DX12_CAPTURE_AFTER_LONG_FRAMES
  void captureAfterLongFrames(int64_t frame_interval_threshold_us, int frames, int capture_count_limit, UINT flags);
#endif
#endif
  void bindlessSetResourceDescriptorNoLock(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
  void bindlessSetResourceDescriptorNoLock(uint32_t slot, Image *texture, ImageViewState view);
  void deferredBindlessSetResourceDescriptorNoLock(uint32_t slot, Image *texture, ImageViewState view);
  void deferredBindlessSetResourceDescriptorNoLock(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
  void bindlessSetSamplerDescriptorNoLock(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

  uint32_t getCurrentSwapchainIndex() const;
  BaseTex *getSwapchainColorTexture(uint32_t swapchain_index);
  BaseTex *getSwapchainSecondaryColorTexture(uint32_t swapchain_index);
  BaseTex *getCurrentSwapchainColorTexture();
  BaseTex *getCurrentSwapchainSecondaryColorTexture();
  Extent2D getSwapchainExtent() const;
  bool isVrrSupported() const;
  bool isVsyncOn() const;
  bool isHfrSupported() const;
  bool isHfrEnabled() const;
  FormatStore getSwapchainColorFormat() const;
  FormatStore getSwapchainSecondaryColorFormat() const;
  DXGISwapChain *getDxgiSwapchain() const;
  bool hasVirtualMainSwapchain() const;
  int getXboxSwapchainFrequency() const;

  // flushes all outstanding work, waits for the backend and GPU to complete it and finish up all
  // outstanding tasks that depend on frame completions
  void finish();

  void registerFrameCompleteEvent(os_event_t event);

  void registerFrameEventCallbacks(FrameEvents *callback, bool useFront);

  void callFrameEndCallbacks();
  void closeFrameEndCallbacks();

  void beginCPUTextureAccess(Image *image);
  void endCPUTextureAccess(Image *image);

  // caller needs to hold the context lock
  void addSwapchainView(Image *image, ImageViewInfo info);

  // caller needs to hold the context lock
  void pushBufferUpdateNoLock(BufferResourceReferenceAndOffset buffer, const void *data, uint32_t data_size);

  void updateFenceProgress();

  void mapTileToResource(BaseTex *tex, ResourceHeap *heap, const TileMapping *mapping, size_t mapping_count);

  void freeUserHeap(::ResourceHeap *ptr);
  void activateBuffer(const BufferReference &buffer, ResourceActivationAction action, GpuPipeline gpu_pipeline);
  void activateTexture(BaseTex *texture, ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline gpu_pipeline);
  void deactivateBuffer(const BufferReference &buffer, GpuPipeline gpu_pipeline);
  void deactivateTexture(Image *tex, GpuPipeline gpu_pipeline);
  void aliasFlush(GpuPipeline gpu_pipeline);
  HostDeviceSharedMemoryRegion allocatePushMemory(uint32_t size, uint32_t alignment);

  void moveBufferNoLock(BufferResourceReferenceAndRange from, BufferResourceReferenceAndOffset to);
  void twoPhaseMoveBufferNoLock(BufferResourceReferenceAndRange from, BufferResourceReferenceAndOffset to,
    const ScratchBuffer &scratch);
  void moveTextureNoLock(Image *from, Image *to);
  void twoPhaseMoveTextureNoLock(Image *from, Image *to, ScratchBuffer scratch_memory);

#if DX12_FIXED_EXECUTION_MODE
  constexpr bool hasWorkerThread() const { return true; }
  constexpr bool enableImmediateFlush() { return false; }
  constexpr void disableImmediateFlush() {}
#else
  bool hasWorkerThread() const
  {
    return executionMode == CommandExecutionMode::CONCURRENT || CommandExecutionMode::CONCURRENT_SYNCED == executionMode;
  }
  bool enableImmediateFlush();
  void disableImmediateFlush();
#endif

  void transitionBuffer(BufferResourceReference buffer, D3D12_RESOURCE_STATES state);

  void resizeImageMipMapTransfer(Image *src, Image *dst, MipMapRange mip_map_range, uint32_t src_mip_map_offset,
    uint32_t dst_mip_map_offset);

  void debugBreak();
  void addDebugBreakString(eastl::string_view str);
  void removeDebugBreakString(eastl::string_view str);

#if !_TARGET_XBOXONE
  void dispatchMesh(uint32_t x, uint32_t y, uint32_t z);
  void dispatchMeshIndirect(BufferResourceReferenceAndOffset args, uint32_t stride, BufferResourceReferenceAndOffset count,
    uint32_t max_count);
#endif
  void addShaderGroup(uint32_t group, ScriptedShadersBinDumpOwner *dump, ShaderID null_pixel_shader, eastl::string_view name);
  void removeShaderGroup(uint32_t group);
  void loadComputeShaderFromDump(ProgramID program);
  void compilePipelineSet2(DynamicArray<InputLayoutIDWithHash> &&input_layouts,
    DynamicArray<StaticRenderStateIDWithHash> &&static_render_states, const DataBlock *output_formats_set,
    const DataBlock *compute_pipeline_set, const DataBlock *full_graphics_set, const DataBlock *null_override_graphics_set,
    const DataBlock *signature, const char *default_format, ShaderID null_pixel_shader);

#if _TARGET_PC_WIN
  void setAsyncPsoCompilationMode(PipelineManager::AsyncPsoMode mode);
#endif

#if D3D_HAS_RAY_TRACING
  void dispatchRays(const ::raytrace::ResourceBindingTable &rbt, const ::raytrace::Pipeline &pipeline,
    const ::raytrace::RayDispatchParameters &rdv);
  void dispatchRaysIndirect(const ::raytrace::ResourceBindingTable &rbt, const ::raytrace::Pipeline &pipeline,
    const ::raytrace::RayDispatchIndirectParameters &rdip);
  void dispatchRaysIndirectCount(const ::raytrace::ResourceBindingTable &rbt, const ::raytrace::Pipeline &pipeline,
    const ::raytrace::RayDispatchIndirectCountParameters &rdicp);

private:
  drv3d_dx12::ResourceBindingTable resolveResourceBindingTable(const ::raytrace::ResourceBindingTable &rbt,
    RayTracePipeline *pipeline);

public:
  void deleteRaytraceAccelerationStructurePool(::raytrace::AccelerationStructurePool pool);
  void deleteRaytraceAccelerationStructure(::raytrace::AccelerationStructurePool pool, ::raytrace::AnyAccelerationStructure structure);
  void accelerationStructurePoolBarrier(::raytrace::AccelerationStructurePool pool);
#endif

  void executeFaultyTextureRead(D3D12_CPU_DESCRIPTOR_HANDLE texture_descriptor);

  void postponeSync();
  void continueSync();

  void setPresentAsyncMode([[maybe_unused]] bool mode)
  {
#if _TARGET_PC_WIN
    isWaitForAsyncPresent.store(!mode);
#endif
  }

  void setGpuPostmortemDataTraceEnabled(bool is_enabled);

#if D3D_HAS_RAY_TRACING
  void setComputeOnRayTraceShaderBindingTableConstBuffer(uint32_t stage, uint32_t slot, ProgramID program);
#endif
};

class ScopedCommitLock
{
  DeviceContext &ctx;

public:
  ScopedCommitLock(DeviceContext &c) : ctx{c} { ctx.beginStateCommit(); }
  ~ScopedCommitLock() { ctx.endStateCommit(); }
  ScopedCommitLock(const ScopedCommitLock &) = delete;
  ScopedCommitLock &operator=(const ScopedCommitLock &) = delete;
  ScopedCommitLock(ScopedCommitLock &&) = delete;
  ScopedCommitLock &operator=(ScopedCommitLock &&) = delete;
};
} // namespace drv3d_dx12
