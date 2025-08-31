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
#include "fsr2_wrapper.h"
#include "gpu_engine_state.h"
#include "info_types.h"
#include "query_manager.h"
#include "resource_memory_heap.h"
#include "resource_state_tracker.h"
#include "ring_pipes.h"
#include "stateful_command_buffer.h"
#include "streamline_adapter.h"
#include "swapchain.h"
#include "tagged_handles.h"
#include "texture.h"
#include "variant_vector.h"
#include "viewport_state.h"
#include "xess_wrapper.h"
#include "synced_command_store.h"

#include <dag/dag_vector.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_renderPass.h>
#include <drv_returnAddrStore.h>
#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_threads.h>


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
    HostDeviceSharedMemoryRegion destiantion;
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
    BufferResourceReferenceAndOffset from;
    BufferResourceReferenceAndRange to;
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
    record.destiantion = cpu_memory;
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
      auto dstOffset = at->destiantion.range.front() + at->offset;
      auto srcOffset = at->source.offset;

      command_list.copyBuffer(at->destiantion.buffer, dstOffset, at->source.buffer, srcOffset, at->source.size);

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

      resource_state.useTextureAsReadBack(barrier_batcher, split_transition_tracker, textureReadBack.image,
        SubresourceIndex::make(textureReadBack.copyInfo.subresourceIndex));

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
      updateProgressFor(bufferReadBack.destiantion, frame_progress);

      auto dstOffset = bufferReadBack.destiantion.range.front() + bufferReadBack.offset;
      auto srcOffset = bufferReadBack.source.offset;
      // only need to do read backs on read back command buffer as buffers decay to common state
      copy_command_list.copyBufferRegion(bufferReadBack.destiantion.buffer, dstOffset, bufferReadBack.source.buffer, srcOffset,
        bufferReadBack.source.size);
    }
    bufferReadBackRecords.clear();

    for (auto &bufferMoveRecord : bufferMoveRecords)
    {
      copy_command_list.copyBufferRegion(bufferMoveRecord.to.buffer, bufferMoveRecord.to.offset, bufferMoveRecord.from.buffer,
        bufferMoveRecord.from.offset, bufferMoveRecord.to.size);
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
  void moveBuffer(BufferResourceReferenceAndOffset from, BufferResourceReferenceAndRange to)
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
  void shutdown(DeviceQueueGroup &queue_group, PipelineManager &pipe_man);
  // returns ticks waiting for gpu
  int64_t beginFrame(DeviceQueueGroup &queue_group, PipelineManager &pipe_man, uint32_t frame_idx);
  void preRecovery(DeviceQueueGroup &queue_group, PipelineManager &pipe_man);
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
      return T{CmdBase{}, this->generateCommandData(), eastl::forward<Args>(args)...};
    }
    else
    {
      return T{CmdBase{}, {}, eastl::forward<Args>(args)...};
    }
  }

  // Warning: intentionally not spinlock, since it has extremely bad behaviour
  // in case of high contention (e.g. during several threads of loading etc...)
  WinCritSec mutex;
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
    struct TextureUploadRecord
    {
      Image *target = nullptr;
      BufferImageCopy region = {};
      HostDeviceSharedMemoryRegion source;
    };

    ForwardRing<FrameInfo, FRAME_FRAME_BACKLOG_LENGTH> frames;
    SignatureStore drawIndirectSignatures;
    SignatureStore drawIndexedIndirectSignatures;
    SignatureStore dispatchRaySignatures;
    ComPtr<ID3D12CommandSignature> dispatchIndirectSignature;
    BarrierBatcher uploadBarrierBatch;
    BarrierBatcher graphicsCommandListBarrierBatch;
    SplitTransitionTracker graphicsCommandListSplitBarrierTracker;
    InititalResourceStateSet initialResourceStateSet;
    ResourceUsageManagerWithHistory resourceStates;
    ResourceActivationTracker resourceActivationTracker;
    BufferAccessTracker bufferAccessTracker;
    ReadBackManager readBackManager;
    ID3D12Resource *lastAliasBegin = nullptr;
    FramebufferLayoutManager framebufferLayouts;
    CopyCommandList<VersionedPtr<D3DCopyCommandList>> activeReadBackCommandList;
    CopyCommandList<AnyCommandListPtr> activePreFrameCommands;
    dag::Vector<D3D12_RESOURCE_BARRIER> pendingPreFrameBarriers;
    CopyCommandList<VersionedPtr<D3DCopyCommandList>> activeFrameConcurrentCommands;
    dag::Vector<TextureUploadRecord> activeTextureUploads;

    backend::BindlessSetManager bindlessSetManager;

    GraphicsState graphicsState;
    ComputeState computeState;
#if D3D_HAS_RAY_TRACING
    RaytraceState raytraceState;
#endif
    PipelineStageStateBase stageState[STAGE_MAX_EXT];

    StatefulCommandBuffer cmdBuffer = {};
    dag::Vector<eastl::pair<size_t, size_t>> renderTargetSplitStarts;

    ActivePipeline activePipeline = ActivePipeline::Graphics;

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
#if D3D_HAS_RAY_TRACING
      raytraceState.onFlush();
#endif

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

    void preRecovery(DeviceQueueGroup &queue_group, PipelineManager &pipe_man);

    void onFrameStateInvalidate(D3D12_CPU_DESCRIPTOR_HANDLE null_ct)
    {
      graphicsState.onFrameStateInvalidate(null_ct);
      computeState.onFrameStateInvalidate();
#if D3D_HAS_RAY_TRACING
      raytraceState.onFrameStateInvalidate();
#endif

      for (auto &stage : stageState)
      {
        stage.resetAllState();
      }

      cmdBuffer.resetForFrameStart();
      G_ASSERT(renderTargetSplitStarts.empty());
      renderTargetSplitStarts.clear();
    }

    void executeTextureUpload(ResourceUsageManagerWithHistory &resource_state, BarrierBatcher &barrier_batcher,
      SplitTransitionTracker &split_transition_tracker, StatefulCommandBuffer &command_list, const TextureUploadRecord &record)
    {
      if (record.target->getGlobalSubResourceIdBase().isValid())
      {
        resource_state.useTextureAsCopyDestination(barrier_batcher, split_transition_tracker, record.target,
          SubresourceIndex::make(record.region.subresourceIndex));
      }

      barrier_batcher.execute(command_list);

      D3D12_TEXTURE_COPY_LOCATION dst = {
        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
      };

      D3D12_TEXTURE_COPY_LOCATION src = {
        .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
      };

      dst.pResource = record.target->getHandle();
      dst.SubresourceIndex = record.region.subresourceIndex;

      src.pResource = record.source.buffer;
      src.PlacedFootprint = record.region.layout;
      src.PlacedFootprint.Offset += record.source.range.front();

      command_list.copyTexture(&dst, record.region.imageOffset.x, record.region.imageOffset.y, record.region.imageOffset.z, &src,
        nullptr);

      // on untracked we simply go back to common state
      if (!record.target->getGlobalSubResourceIdBase().isValid())
      {
        barrier_batcher.transition(dst.pResource, SubresourceIndex::make(dst.SubresourceIndex), D3D12_RESOURCE_STATE_COPY_DEST,
          D3D12_RESOURCE_STATE_COMMON);
      }
    }

    template <typename Cmp>
    void onTextureAccess(ResourceUsageManagerWithHistory &resource_state, BarrierBatcher &barrier_batcher,
      SplitTransitionTracker &split_transition_tracker, StatefulCommandBuffer &command_list, Image *target, Cmp &&cmp)
    {
      // textures with global id are never queued to upload
      if (target->getGlobalSubResourceIdBase().isValid())
      {
        return;
      }
      auto at = eastl::find_if(activeTextureUploads.begin(), activeTextureUploads.end(), cmp);
      while (at != activeTextureUploads.end())
      {
        executeTextureUpload(resource_state, barrier_batcher, split_transition_tracker, command_list, *at);
        // get the offset and use it for next search so we are not searching again over the same span we know it has no matching
        // entries
        auto ofs = at - activeTextureUploads.begin();
        // have to keep ordering to be safe so that repeated uploads are applied correctly
        activeTextureUploads.erase(at);
        at = eastl::find_if(activeTextureUploads.begin() + ofs, activeTextureUploads.end(), cmp);
      }
    }

    void onTextureAccess(ResourceUsageManagerWithHistory &resource_state, BarrierBatcher &barrier_batcher,
      SplitTransitionTracker &split_transition_tracker, StatefulCommandBuffer &command_list, Image *target)
    {
      onTextureAccess(resource_state, barrier_batcher, split_transition_tracker, command_list, target,
        [target](auto &record) { return target == record.target; });
    }

    void onTextureAccess(ResourceUsageManagerWithHistory &resource_state, BarrierBatcher &barrier_batcher,
      SplitTransitionTracker &split_transition_tracker, StatefulCommandBuffer &command_list, Image *target, SubresourceIndex sub_res)
    {
      onTextureAccess(resource_state, barrier_batcher, split_transition_tracker, command_list, target,
        [target, sub_res](auto &record) { return target == record.target && sub_res.index() == record.region.subresourceIndex; });
    }

    void onTextureAccess(ResourceUsageManagerWithHistory &resource_state, BarrierBatcher &barrier_batcher,
      SplitTransitionTracker &split_transition_tracker, StatefulCommandBuffer &command_list, Image *target,
      SubresourceRange sub_res_range)
    {
      onTextureAccess(resource_state, barrier_batcher, split_transition_tracker, command_list, target,
        [target, sub_res_range](auto &record) {
          return target == record.target && sub_res_range.isInside(SubresourceIndex::make(record.region.subresourceIndex));
        });
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
    static unsigned on_ext_call_stack_resolve(char *buf, unsigned max_buf, stackhelp::CallStackInfo stack);

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
    void buildTopAccelerationStructure(uint32_t batch_size, uint32_t batch_index, uint32_t instance_count,
      BufferResourceReferenceAndAddress instance_buffer, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, bool update,
      RaytraceAccelerationStructure *dst, RaytraceAccelerationStructure *src, BufferResourceReferenceAndAddress scratch);
    void copyRaytracingAccelerationStructure(RaytraceAccelerationStructure *dst, RaytraceAccelerationStructure *src, bool compact);
#endif
    void continuePipelineSetCompilation() const;
    void finishFrame(uint64_t progress, Drv3dTimings *timing_data, int64_t kickoff_stamp, uint32_t front_frame, uint32_t frame_id,
      const PresentInfo &present_info);
    void dispatch(uint32_t x, uint32_t y, uint32_t z);
    void dispatchIndirect(BufferResourceReferenceAndOffset buffer);
    void copyBuffer(BufferResourceReferenceAndOffset src, BufferResourceReferenceAndOffset dst, uint32_t size);
    void updateBuffer(HostDeviceSharedMemoryRegion update, BufferResourceReferenceAndOffset dest);
    void clearBufferFloat(BufferResourceReferenceAndClearView buffer, const float values[4]);
    void clearBufferUint(BufferResourceReferenceAndClearView buffer, const uint32_t values[4]);
    void clearDepthStencilImage(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor,
      const ClearDepthStencilValue &value, const eastl::optional<D3D12_RECT> &rect);
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
    void bindVertexUserData(HostDeviceSharedMemoryRegion bsa, uint32_t stride);
    void drawIndirect(BufferResourceReferenceAndOffset buffer, uint32_t count, uint32_t stride);
    void drawIndexedIndirect(BufferResourceReferenceAndOffset buffer, uint32_t count, uint32_t stride);
    void draw(uint32_t count, uint32_t instance_count, uint32_t start, uint32_t first_instance);
    void drawIndexed(uint32_t count, uint32_t instance_count, uint32_t index_start, int32_t vertex_base, uint32_t first_instance);
    void bindIndexUser(HostDeviceSharedMemoryRegion bsa);
    void flushViewportAndScissor();
    void flushGraphicsResourceBindings();
    void flushGraphicsMeshState();
    void flushGraphicsState(D3D12_PRIMITIVE_TOPOLOGY top);
    void flushIndexBuffer();
    void flushVertexBuffers();
    void flushStreamOutputBuffer();
    void resetStreamOutputBufferOnFlush();
    void flushGraphicsStateResourceBindings();
    // handled by flush, could be moved into this though
    void ensureActivePass();
    void changePresentMode(PresentationMode mode);
    void updateVertexShaderName(ShaderID shader, StringIndexRef::RangeType name);
    void updatePixelShaderName(ShaderID shader, StringIndexRef::RangeType name);
    void clearUAVTextureI(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor, const uint32_t values[4]);
    void clearUAVTextureF(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor, const float values[4]);
    void setRootConstants(unsigned stage, const RootConstatInfo &values);
    void registerStaticRenderState(StaticRenderStateID ident, const RenderStateSystem::StaticState &state);
    void beginVisibilityQuery(Query *q);
    void endVisibilityQuery(Query *q);
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
    void releaseDlssFeature(bool stereo_render);
    void executeDlss(const nv::DlssParams<Image> &dlss_params, int view_index);
    void executeDlssG(const nv::DlssGParams<Image> &dlss_g_params, int view_index);
    void prepareExecuteAA(std::initializer_list<Image *> inputs, std::initializer_list<Image *> outputs);
    void executeXess(const XessParamsDx12 &params);
    void executeXeFg(const XessFgParamsDx12 &params);
    void executeFSR(amd::FSR *fsr, const FSRUpscalingArgs &params);
    void executeFSR2(const Fsr2ParamsDx12 &params);
    void executeFSRFG(amd::FSR *fsr, const FSRFrameGenArgs &params);
    void removeVertexShader(ShaderID shader);
    void removePixelShader(ShaderID shader);
    void deleteProgram(ProgramID program);
    void deleteGraphicsProgram(GraphicsProgramID program);
    void deleteQueries(QueryPointerListRef::RangeType queries);
    void hostToDeviceMemoryCopy(BufferResourceReferenceAndRange target, HostDeviceSharedMemoryRegion source, size_t source_offset);
    void hostToDeviceTextureCopy(ContextState::TextureUploadRecord upload);
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
    void bindlessSetSamplerDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
    void bindlessCopyResourceDescriptors(uint32_t src, uint32_t dst, uint32_t count);

    void registerFrameCompleteEvent(os_event_t event);

    void registerFrameEventsCallback(FrameEvents *callback);
#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
    void resetBufferState(BufferResourceReference buffer);
#endif
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
    void commandFence(std::atomic_bool &signal);
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
    void moveBuffer(BufferResourceReferenceAndOffset from, BufferResourceReferenceAndRange to);
    void moveTexture(Image *from, Image *to);

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
    void compilePipelineSet(DynamicArray<InputLayoutIDWithHash> &&input_layouts,
      DynamicArray<StaticRenderStateIDWithHash> &&static_render_states, DynamicArray<FramebufferLayoutWithHash> &&framebuffer_layouts,
      DynamicArray<GraphicsPipelinePreloadInfo> &&graphics_pipelines, DynamicArray<MeshPipelinePreloadInfo> &&mesh_pipelines,
      DynamicArray<ComputePipelinePreloadInfo> &&compute_pipelines);
    void compilePipelineSet(DynamicArray<InputLayoutIDWithHash> &&input_layouts,
      DynamicArray<StaticRenderStateIDWithHash> &&static_render_states, DynamicArray<FramebufferLayoutWithHash> &&framebuffer_layouts,
      DynamicArray<cacheBlk::SignatureEntry> &&scripted_shader_dump_signature,
      DynamicArray<cacheBlk::ComputeClassUse> &&compute_pipelines, DynamicArray<cacheBlk::GraphicsVariantGroup> &&graphics_pipelines,
      DynamicArray<cacheBlk::GraphicsVariantGroup> &&graphics_with_null_override_pipelines, ShaderID null_pixel_shader);

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
  };


  std::atomic<uint32_t> finishedFrameIndex = 0;

  struct FrontendFrameLatchedData
  {
    uint64_t progress = 0;
#if DX12_RECORD_TIMING_DATA
    uint32_t frameIndex = 0;
#endif
    dag::Vector<Query *> deletedQueries;
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
      UINT flags = 0x10000 /* D3D11X_PIX_CAPTURE_API */;
      int frameCount = 1;
      int captureCountLimit = -1;
      int ignoreNextFrames = 5;
    } captureAfterLongFrames;
#endif
#endif
    alignas(std::hardware_constructive_interference_size) std::atomic_uint32_t waitForSwapchainCount = 0;
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
  WIN_MEMBER bool isWaitForASyncPresent = false;
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
#if !_TARGET_XBOX
  eastl::optional<StreamlineAdapter> streamlineAdapter;
#endif
  Fsr2Wrapper fsr2Wrapper;

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

  void replayCommandsWithProfiler(ExecutionContext &execution_context);
  bool replayCommands(ExecutionContext &execution_context);
  void replayCommandsConcurrently(volatile int &terminate);

  enum class TidyFrameMode
  {
    FrameCompleted,
    SyncPoint,
  };
  void frontFlush(TidyFrameMode tidy_mode);
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
  void draw(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t start, uint32_t count, uint32_t first_instance, uint32_t instance_count);
  // size of vertex_data = count * stride
  void drawUserData(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t count, uint32_t stride, const void *vertex_data);
  void drawIndexed(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t index_start, uint32_t count, int32_t vertex_base, uint32_t first_instance,
    uint32_t instance_count);
  // size of vertex_data = vertex_stride * vertex_count
  // size of index_data = count * uint16_t
  void drawIndexedUserData(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t count, uint32_t vertex_stride, const void *vertex_data,
    uint32_t vertex_count, const void *index_data);
  void setComputePipeline(ProgramID program);
  void setGraphicsPipeline(GraphicsProgramID program);
  void copyBuffer(BufferResourceReferenceAndOffset source, BufferResourceReferenceAndOffset dest, uint32_t data_size);
  void updateBuffer(HostDeviceSharedMemoryRegion update, BufferResourceReferenceAndOffset dest);
  void clearBufferFloat(BufferResourceReferenceAndClearView buffer, const float values[4]);
  void clearBufferInt(BufferResourceReferenceAndClearView buffer, const unsigned values[4]);
  void pushEvent(const char *name);
  void popEvent();
  void updateViewports(dag::ConstSpan<ViewportState> viewports);
  void clearDepthStencilImage(Image *image, const ImageSubresourceRange &area, const ClearDepthStencilValue &value,
    const eastl::optional<D3D12_RECT> &rect);
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
  BufferState discardBuffer(BufferState to_discared, DeviceMemoryClass memory_class, FormatStore format, uint32_t struct_size,
    bool raw_view, bool struct_view, D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name);
  void checkFramebufferIntegityNoLock(Image *img);
  void destroyImage(Image *img, bool is_rt);
  // blocks the main thread until the previously started swapchain present on the backend thread finishes
  void waitForAsyncPresent();
  // blocks the input sampling until the GPU finished the running frame
  void gpuLatencyWait();
  void beginFrame(uint32_t frame_id, bool allow_wait);
  void setLatencyMarker(uint32_t frame_id, lowlatency::LatencyMarkerType type);
  void finishFrame(uint32_t frame_id, bool present_on_swapchain = true);
  void changePresentMode(PresentationMode mode);
  void changeCurrentSwapchainExtents(Extent2D size);
#if _TARGET_PC_WIN
  void changePresentWindowNoLock(uint32_t index);
  void changeSwapchainExtents(Extent2D size, uint32_t swapchian_index);
  HRESULT getSwapchainDesc(DXGI_SWAP_CHAIN_DESC *out_desc) const;
  IDXGIOutput *getSwapchainOutput() const;
#endif
  void shutdownSwapchain();
  void insertTimestampQuery(Query *query);
  void deleteQuery(Query *query);
  void generateMipmaps(Image *img);
  void setFramebuffer(Image **image_list, ImageViewState *view_list, bool read_only_depth);
#if D3D_HAS_RAY_TRACING
  void raytraceBuildBottomAccelerationStructure(uint32_t batch_size, uint32_t batch_index, RaytraceBottomAccelerationStructure *as,
    const RaytraceGeometryDescription *desc, uint32_t count, RaytraceBuildFlags flags, bool update,
    BufferResourceReferenceAndAddress scratch_buf, BufferResourceReferenceAndAddress compacted_size);
  void raytraceBuildTopAccelerationStructure(uint32_t batch_size, uint32_t batch_index, RaytraceTopAccelerationStructure *as,
    BufferReference instance_buffer, uint32_t instance_count, RaytraceBuildFlags flags, bool update,
    BufferResourceReferenceAndAddress scratch_buf);
  void raytraceCopyAccelerationStructure(RaytraceAccelerationStructure *dst, RaytraceAccelerationStructure *src, bool compact);
  void deleteRaytraceBottomAccelerationStructure(RaytraceBottomAccelerationStructure *desc);
  void deleteRaytraceTopAccelerationStructure(RaytraceTopAccelerationStructure *desc);
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
  void preRecovery();
  void recover(const dag::Vector<D3D12_CPU_DESCRIPTOR_HANDLE> &unbounded_samplers);
#endif
  void deleteTexture(BaseTex *tex);
  void resetBindlessReferences(BaseTex *tex);
  void resetBindlessReferences(BufferState &buffer);
  void removeBindlessReferences(BaseTex *tex);
  void removeBindlessReferences(BufferState &buffer);
  void updateBindlessReferences(D3D12_CPU_DESCRIPTOR_HANDLE old_descriptor, D3D12_CPU_DESCRIPTOR_HANDLE new_descriptor);
  void freeMemory(HostDeviceSharedMemoryRegion allocation);
  void freeMemoryOfUploadBuffer(HostDeviceSharedMemoryRegion allocation);
  void uploadToBuffer(BufferResourceReferenceAndRange target, HostDeviceSharedMemoryRegion memory, size_t m_offset);
  void readBackFromBuffer(HostDeviceSharedMemoryRegion memory, size_t m_offset, BufferResourceReferenceAndRange source);
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
  void initStreamline(ComPtr<DXGIFactory> &factory, DXGIAdapter *adapter);
  void initXeSS();
  void initFsr2();
  void initDLSS();
  void shutdownStreamline();
  void shutdownXess();
  void shutdownFsr2();
  void shutdownDLSS();
  XessState getXessState() const { return xessWrapper.getXessState(); }
  Fsr2State getFsr2State() const { return fsr2Wrapper.getFsr2State(); }

  bool isXeFGSupported() const { return xessWrapper.isFrameGenerationSupported(); }
  bool isXeFGEnabled() const { return xessWrapper.isFrameGenerationEnabled(); }
  void enableXeFG(bool enable) { xessWrapper.enableFrameGeneration(enable); }
  void suppressXeFG(bool suppress) { xessWrapper.suppressFrameGeneration(suppress); }
  void scheduleXeFG(const XessFgParams &params);
  int getXeFgPresentedFrameCount() { return xessWrapper.getPresentedFrameCount(); }

  void shutdownInternalSwapchain();
  bool adoptUserSwapchain(DXGISwapChain *swapchain, SwapchainCreateInfo &&sci);
  bool createDefaultSwapchain(DXGIFactory *factory, SwapchainCreateInfo &&sci);

  void preRecoverStreamline();
  void recoverStreamline(DXGIAdapter *adapter);
#if !_TARGET_XBOX
  nv::Streamline *getStreamline() { return streamlineAdapter ? &streamlineAdapter.value() : nullptr; }
#endif
  bool isXessQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int xess_quality) const
  {
    return xessWrapper.isXessQualityAvailableAtResolution(target_width, target_height, xess_quality);
  }
  void getXessRenderResolution(int &w, int &h) const { xessWrapper.getXeSSRenderResolution(w, h); }
  dag::Expected<eastl::string, XessWrapper::ErrorKind> getXessVersion() const { return xessWrapper.getVersion(); }
  void getFsr2RenderResolution(int &width, int &height) { fsr2Wrapper.getFsr2RenderingResolution(width, height); }
  void setXessVelocityScale(float x, float y) { xessWrapper.setVelocityScale(x, y); }
  void createDlssFeature(bool stereo_render, int output_width, int output_height);
  void releaseDlssFeature(bool stereo_render);
  void executeDlss(const nv::DlssParams<BaseTexture> &dlss_params, int view_index);
  void executeDlssG(const nv::DlssGParams<BaseTexture> &dlss_g_params, int view_index);
  void executeXess(const XessParams &params);
  void executeFSR(amd::FSR *fsr, const amd::FSR::UpscalingArgs &params);
  void executeFSR2(const Fsr2Params &params);
  void executeFSRFG(amd::FSR *fsr, const amd::FSR::FrameGenArgs &params);
  void bufferBarrier(BufferResourceReference buffer, ResourceBarrier barrier, GpuPipeline queue);
  void textureBarrier(Image *tex, SubresourceRange sub_res_range, uint32_t tex_flags, ResourceBarrier barrier, GpuPipeline queue,
    bool force_barrier);
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
  void bindlessSetSamplerDescriptorNoLock(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
  void bindlessCopyResourceDescriptorsNoLock(uint32_t src, uint32_t dst, uint32_t count);

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
  void pushBufferUpdate(BufferResourceReferenceAndOffset buffer, const void *data, uint32_t data_size);

  void updateFenceProgress();

  void mapTileToResource(BaseTex *tex, ResourceHeap *heap, const TileMapping *mapping, size_t mapping_count);

  void freeUserHeap(::ResourceHeap *ptr);
  void activateBuffer(const BufferReference &buffer, ResourceActivationAction action, GpuPipeline gpu_pipeline);
  void activateTexture(BaseTex *texture, ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline gpu_pipeline);
  void deactivateBuffer(const BufferReference &buffer, GpuPipeline gpu_pipeline);
  void deactivateTexture(Image *tex, GpuPipeline gpu_pipeline);
  void aliasFlush(GpuPipeline gpu_pipeline);
  HostDeviceSharedMemoryRegion allocatePushMemory(uint32_t size, uint32_t alignment);

  void moveBuffer(BufferResourceReferenceAndOffset from, BufferResourceReferenceAndRange to);
  void moveTextureNoLock(Image *from, Image *to);

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
  void compilePipelineSet(DynamicArray<InputLayoutIDWithHash> &&input_layouts,
    DynamicArray<StaticRenderStateIDWithHash> &&static_render_states, const DataBlock *output_formats_set,
    const DataBlock *graphics_pipeline_set, const DataBlock *mesh_pipeline_set, const DataBlock *compute_pipeline_set,
    const char *default_format);
  void compilePipelineSet2(DynamicArray<InputLayoutIDWithHash> &&input_layouts,
    DynamicArray<StaticRenderStateIDWithHash> &&static_render_states, const DataBlock *output_formats_set,
    const DataBlock *compute_pipeline_set, const DataBlock *full_graphics_set, const DataBlock *null_override_graphics_set,
    const DataBlock *signature, const char *default_format, ShaderID null_pixel_shader);

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
