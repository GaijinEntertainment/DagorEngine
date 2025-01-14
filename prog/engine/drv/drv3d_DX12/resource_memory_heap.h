// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/fixed_vector.h>
#include <free_list_utils.h>
#include <value_range.h>

#if _TARGET_XBOX
#define REPORT_HEAP_INFO 1
#endif

#if REPORT_HEAP_INFO
#define HEAP_LOG(...) logdbg(__VA_ARGS__)
#else
#define HEAP_LOG(...)
#endif

#include "resource_manager/object_components.h"
#include "resource_manager/descriptor_components.h"
#include "resource_manager/heap_components.h"
#include "resource_manager/esram_components.h"
#include "resource_manager/host_shared_components.h"
#include "resource_manager/buffer_components.h"
#include "resource_manager/rtx_components.h"

struct ResourceDumpTexture;
struct ResourceDumpBuffer;
struct ResourceDumpRayTrace;
typedef eastl::variant<ResourceDumpTexture, ResourceDumpBuffer, ResourceDumpRayTrace> ResourceDumpInfo;

namespace drv3d_dx12
{
struct ScratchBuffer
{
  ID3D12Resource *buffer = nullptr;
  size_t offset = 0;

  explicit operator bool() const { return nullptr != buffer; }
};
namespace resource_manager
{
class TextureImageFactory : public RaytraceAccelerationStructureObjectProvider
{
  using BaseType = RaytraceAccelerationStructureObjectProvider;

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    dag::Vector<Image *> deletedImages;
  };

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    destroyTextures({begin(data.deletedImages), end(data.deletedImages)}, *info.bindlessManager);
    data.deletedImages.clear();
    BaseType::completeFrameExecution(info, data);
  }

  void freeView(const ImageViewInfo &view);

public:
  ImageCreateResult createTexture(DXGIAdapter *adapter, Device &device, const ImageInfo &ii, const char *name);
  Image *adoptTexture(ID3D12Resource *texture, const char *name);

  void destroyTextureOnFrameCompletion(Image *texture)
  {
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>([=](auto &data) { data.deletedImages.push_back(texture); });
  }

  bool tryDestroyTextureOnFrameCompletion(Image *texture)
  {
    bool isAlreadyRemoved = false;
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>([=, &isAlreadyRemoved](auto &data) {
      auto it = eastl::find(data.deletedImages.begin(), data.deletedImages.end(), texture);
      if (it != data.deletedImages.end())
      {
        isAlreadyRemoved = true;
        return;
      }
      data.deletedImages.push_back(texture);
    });
    return !isAlreadyRemoved;
  }

  Image *tryCloneTexture(DXGIAdapter *adapter, ID3D12Device *device, Image *original, D3D12_RESOURCE_STATES initialState,
    AllocationFlags allocation_flags);

protected:
  void destroyTextures(eastl::span<Image *> textures, frontend::BindlessManager &bindless_manager);
};

class AliasHeapProvider : public TextureImageFactory
{
  using BaseType = TextureImageFactory;

  struct AliasHeap
  {
    enum class Flag
    {
      AUTO_FREE,
      COUNT
    };
    ResourceMemory memory;
    dag::Vector<Image *> images;
    dag::Vector<BufferGlobalId> buffers;
    TypedBitSet<Flag> flags;
#if _TARGET_XBOX
    HANDLE heapRegHandle = nullptr;
#endif

    explicit operator bool() const { return static_cast<bool>(memory); }

    void reset() { memory = {}; }

    bool shouldBeFreed() const { return flags.test(Flag::AUTO_FREE) && images.empty() && buffers.empty(); }
  };

  static D3D12_RESOURCE_DESC as_desc(const BasicTextureResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const TextureResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const VolTextureResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const ArrayTextureResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const CubeTextureResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const ArrayCubeTextureResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const BufferResourceDescription &desc);
  static D3D12_RESOURCE_DESC as_desc(const ResourceDescription &desc);
  // TODO: currently wrong naming convention (was static in the past), need to be fixed at some point.
  DeviceMemoryClass get_memory_class(const ResourceDescription &desc);

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    dag::Vector<::ResourceHeap *> deletedResourceHeaps;
  };

  ContainerMutexWrapper<dag::Vector<AliasHeap>, OSSpinlock> aliasHeaps;

  uint32_t adoptMemoryAsAliasHeap(ResourceMemory memory);

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    bool needsAutoFreeHandling = detachTextures({begin(data.deletedImages), end(data.deletedImages)});

    for (auto &&freeInfo : data.deletedBuffers)
    {
      needsAutoFreeHandling = detachBuffer(freeInfo.buffer) || needsAutoFreeHandling;
    }

    BaseType::completeFrameExecution(info, data);
    for (auto heap : data.deletedResourceHeaps)
    {
      freeUserHeap(info.device, heap);
    }
    data.deletedResourceHeaps.clear();
    if (needsAutoFreeHandling)
    {
      processAutoFree();
    }
  }

public:
  ::ResourceHeap *newUserHeap(DXGIAdapter *adapter, Device &device, ::ResourceHeapGroup *group, size_t size,
    ResourceHeapCreateFlags flags);
  // assumes mutex is locked
  void freeUserHeap(ID3D12Device *device, ::ResourceHeap *ptr);
  ResourceAllocationProperties getResourceAllocationProperties(ID3D12Device *device, const ResourceDescription &desc);
  ImageCreateResult placeTextureInHeap(DXGIAdapter *adapter, ID3D12Device *device, ::ResourceHeap *heap,
    const ResourceDescription &desc, size_t offset, const ResourceAllocationProperties &alloc_info, const char *name);
  ResourceHeapGroupProperties getResourceHeapGroupProperties(::ResourceHeapGroup *heap_group);

  ResourceMemory getUserHeapMemory(::ResourceHeap *heap);

protected:
  bool detachTextures(eastl::span<Image *> textures);
  void processAutoFree();

public:
  BufferState placeBufferInHeap(DXGIAdapter *adapter, ID3D12Device *device, ::ResourceHeap *heap, const ResourceDescription &desc,
    size_t offset, const ResourceAllocationProperties &alloc_info, const char *name);
  bool detachBuffer(const BufferState &buf);

  ImageCreateResult aliasTexture(DXGIAdapter *adapter, ID3D12Device *device, const ImageInfo &ii, Image *base, const char *name);

  void freeUserHeapOnFrameCompletion(::ResourceHeap *ptr)
  {
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>([=](auto &data) { data.deletedResourceHeaps.push_back(ptr); });
  }

  template <typename T>
  void visitAliasingHeaps(T clb)
  {
    auto heapAccess = aliasHeaps.access();
    clb.beginVisit();
    for (uint32_t i = 0; i < heapAccess->size(); ++i)
    {
      const auto &heap = (*heapAccess)[i];
      if (!heap)
      {
        continue;
      }
      clb.visitAliasingHeap(i, heap.memory, heap.images, heap.buffers, heap.flags.test(AliasHeap::Flag::AUTO_FREE));
    }
    clb.endVisit();
  }
};

class ScratchBufferProvider : public AliasHeapProvider
{
  using BaseType = AliasHeapProvider;

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    dag::Vector<BasicBuffer> deletedScratchBuffers;
  };

  struct TempScratchBufferState
  {
    BasicBuffer buffer = {};
    size_t allocatedSpace = 0;

    bool ensureSize(DXGIAdapter *adapter, ID3D12Device *device, size_t size, size_t alignment, ScratchBufferProvider *heap)
    {
      auto spaceNeeded = align_value(allocatedSpace, alignment) + size;
      if (spaceNeeded <= buffer.bufferMemory.size())
      {
        return true;
      }
      // default to times 2 so we stop to realloc at some point
      auto nextSize = max<size_t>(scartch_buffer_min_size, buffer.bufferMemory.size() * 2);
      // out of space
      if (buffer)
      {
        heap->accessRecodingPendingFrameCompletion<ScratchBufferProvider::PendingForCompletedFrameData>(
          [&](auto &data) { data.deletedScratchBuffers.push_back(eastl::move(buffer)); });

        buffer = {};
      }

      while (nextSize < size)
      {
        nextSize += nextSize;
      }

      nextSize = align_value<size_t>(nextSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

      D3D12_RESOURCE_DESC desc{};
      desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
      desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
      desc.Width = nextSize;
      desc.Height = 1;
      desc.DepthOrArraySize = 1;
      desc.MipLevels = 1;
      desc.Format = DXGI_FORMAT_UNKNOWN;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
      desc.Flags = D3D12_RESOURCE_FLAG_NONE;

      D3D12_RESOURCE_ALLOCATION_INFO allocInfo{};
      allocInfo.SizeInBytes = desc.Width;
      allocInfo.Alignment = desc.Alignment;

      auto memoryProperties = heap->getScratchBufferHeapProperties();
      auto memory = heap->allocate(adapter, device, memoryProperties, allocInfo, {});
      if (!memory)
      {
        return false;
      }

      auto errorCode = buffer.create(device, desc, memory, D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE, false);

      allocatedSpace = 0;
      if (DX12_CHECK_OK(errorCode))
      {
        heap->recordScratchBufferAllocated(desc.Width);
        heap->updateMemoryRangeUse(memory, ScratchBufferReference{buffer.buffer.Get()});
      }
      else
      {
        heap->free(memory, false);
      }

      return DX12_CHECK_OK(errorCode);
    }

    ScratchBuffer getSpace(DXGIAdapter *adapter, ID3D12Device *device, size_t size, size_t alignment, ScratchBufferProvider *heap)
    {
      ScratchBuffer result{};
      ensureSize(adapter, device, size, alignment, heap);
      if (buffer)
      {
        result.buffer = buffer.buffer.Get();
        result.offset = align_value(allocatedSpace, alignment);
        // keep allocatedSpace as it is only used for one operation and can be reused on the next
      }
      return result;
    }

    ScratchBuffer getPersistentSpace(DXGIAdapter *adapter, ID3D12Device *device, size_t size, size_t alignment,
      ScratchBufferProvider *heap)
    {
      auto space = getSpace(adapter, device, size, alignment, heap);
      if (space)
      {
        // alter allocatedSpace to turn this transient space into persistent
        allocatedSpace = space.offset + size;
      }
      return space;
    }

    void reset(ScratchBufferProvider *heap)
    {
      if (buffer)
      {
        buffer.reset(heap);
      }
    }
  };

  using ScratchBufferWrapper = ContainerMutexWrapper<TempScratchBufferState, OSSpinlock>;
  ScratchBufferWrapper tempScratchBufferState;

  static constexpr size_t scartch_buffer_min_size = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

protected:
  BasicBuffer allocateNewScratchBuffer(DXGIAdapter *adapter, ID3D12Device *device, uint32_t size, AllocationFlags allocation_flags,
    ScratchBufferWrapper::AccessToken &scratchBufferAccess)
  {
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_RESOURCE_ALLOCATION_INFO allocInfo{};
    allocInfo.SizeInBytes = desc.Width;
    allocInfo.Alignment = desc.Alignment;

    auto memoryProperties =
      getProperties(D3D12_RESOURCE_FLAG_NONE, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
    auto memory = allocate(adapter, device, memoryProperties, allocInfo, allocation_flags);
    BasicBuffer newBuffer;
    if (!memory)
    {
      return newBuffer;
    }
    const auto errorCode = newBuffer.create(device, desc, memory, D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE, false);
    if (DX12_CHECK_OK(errorCode))
    {
      recordScratchBufferAllocated(desc.Width);
      updateMemoryRangeUse(memory, ScratchBufferReference{scratchBufferAccess->buffer.buffer.Get()});
    }
    else
      free(memory, allocation_flags.test(AllocationFlag::DEFRAGMENTATION_OPERATION));
    return newBuffer;
  }

  bool tryMoveScratchBuffer(ScratchBufferWrapper::AccessToken &scratchBufferAccess, DXGIAdapter *adapter, ID3D12Device *device,
    AllocationFlags allocation_flags)
  {
    uint32_t size = scratchBufferAccess->buffer.bufferMemory.size();
    auto movedBuffer = allocateNewScratchBuffer(adapter, device, size, allocation_flags, scratchBufferAccess);
    if (!movedBuffer)
    {
      return false;
    }
    // queue current buffer for deletion and replace it with the new one
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
      [=, &scratchBufferAccess](auto &data) { data.deletedScratchBuffers.push_back(eastl::move(scratchBufferAccess->buffer)); });
    scratchBufferAccess->buffer = eastl::move(movedBuffer);
    scratchBufferAccess->allocatedSpace = 0;
    return true;
  }

  void shutdown()
  {
    tempScratchBufferState.access()->reset(this);
    BaseType::shutdown();
  }

  void preRecovery()
  {
    tempScratchBufferState.access()->reset(this);
    BaseType::preRecovery();
  }

public:
  // Memory is used for one operation an can be reused by the next (for inter resource copy for
  // example)
  ScratchBuffer getTempScratchBufferSpace(DXGIAdapter *adapter, Device &device, size_t size, size_t alignment);

  // Memory can be used anytime by multiple distinct operations until the frame has ended.
  ScratchBuffer getPersistentScratchBufferSpace(DXGIAdapter *adapter, Device &device, size_t size, size_t alignment);

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    for (auto &buffer : data.deletedScratchBuffers)
    {
      recordScratchBufferFreed(buffer.bufferMemory.size());
      buffer.reset(this);
    }
    data.deletedScratchBuffers.clear();

    BaseType::completeFrameExecution(info, data);
  }

  void completeFrameRecording(const CompletedFrameRecordingInfo &info)
  {
    BaseType::completeFrameRecording(info);
    tempScratchBufferState.access()->allocatedSpace = 0;
  }

private:
  ResourceHeapProperties getScratchBufferHeapProperties()
  {
    return getProperties(D3D12_RESOURCE_FLAG_NONE, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER,
      D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
  }
};

class SamplerDescriptorProvider : public ScratchBufferProvider
{
  using BaseType = ScratchBufferProvider;

  using SamplerInfo = SamplerDescriptorAndState;

  struct State
  {
    dag::Vector<SamplerInfo> samplers;
    DescriptorHeap<SamplerStagingPolicy> descriptors;

    dag::Vector<SamplerInfo>::const_iterator get(ID3D12Device *device, SamplerState state)
    {
      auto ref = eastl::find_if(begin(samplers), end(samplers), [state](auto &info) { return state == info.state; });
      if (ref == end(samplers))
      {
        SamplerInfo sampler{descriptors.allocate(device), state};

        D3D12_SAMPLER_DESC desc = state.asDesc();
        device->CreateSampler(&desc, sampler.sampler);

        ref = samplers.insert(ref, sampler);
      }
      return ref;
    }
  };
  ContainerMutexWrapper<State, OSSpinlock> samplers;

  template <typename T>
  void accessSampler(d3d::SamplerHandle handle, T clb)
  {
    auto index = uint64_t(handle) - 1;
    auto access = samplers.access();
    clb(access->samplers[index]);
  }

protected:
  template <typename T>
  void visitSamplerObjects(T clb)
  {
    auto access = samplers.access();
    for (auto &info : access->samplers)
    {
      auto handle = uint64_t((&info - access->samplers.data()) + 1);
      clb(handle, const_cast<const SamplerInfo &>(info));
    }
  }
  void setup(const SetupInfo &setup)
  {
    BaseType::setup(setup);

    auto access = samplers.access();
    access->descriptors.init(setup.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));
    // if we are restoring try to restore all samplers
    for (auto &info : access->samplers)
    {
      D3D12_SAMPLER_DESC desc = info.state.asDesc();
      info.sampler = access->descriptors.allocate(setup.device);
      setup.device->CreateSampler(&desc, info.sampler);
    }
  }

  void shutdown()
  {
    BaseType::shutdown();

    auto access = samplers.access();
    access->samplers.clear();
    access->descriptors.shutdown();
  }

  void preRecovery()
  {
    BaseType::preRecovery();

    auto access = samplers.access();
    access->descriptors.shutdown();
    // keep sampler list as we can restore it later
  }

  d3d::SamplerHandle createSampler(ID3D12Device *device, SamplerState state)
  {
    state.normalizeSelf();

    auto access = samplers.access();
    auto ref = access->get(device, state);
    uint64_t index = ref - begin(access->samplers) + 1;
    return d3d::SamplerHandle(index);
  }
  D3D12_CPU_DESCRIPTOR_HANDLE getSampler(d3d::SamplerHandle handle)
  {
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor{};
    accessSampler(handle, [&descriptor](auto &info) { descriptor = info.sampler; });
    return descriptor;
  }
  D3D12_CPU_DESCRIPTOR_HANDLE getSampler(ID3D12Device *device, SamplerState state)
  {
    state.normalizeSelf();

    auto access = samplers.access();
    auto ref = access->get(device, state);
    return ref->sampler;
  }
  SamplerDescriptorAndState getSamplerInfo(d3d::SamplerHandle handle)
  {
    SamplerDescriptorAndState result{};
    accessSampler(handle, [&result](auto &info) { result = info; });
    return result;
  }
};

#define DEFRAG_LOGGING_ENABLED 0

template <class... ARG_TYPE>
void defragLogdbg(bool is_emergency_defragmentation, ARG_TYPE... args)
{
  bool isRegularDefragmentationLogsEnabled = false;
#if DEFRAG_LOGGING_ENABLED
  isRegularDefragmentationLogsEnabled = true;
#endif
  if (isRegularDefragmentationLogsEnabled || is_emergency_defragmentation)
    logdbg(args...);
}

template <class... ARG_TYPE>
void defragLogdbg(ARG_TYPE... args)
{
  defragLogdbg(false, args...);
}

#define DEFRAG_VERBOSE(...) defragLogdbg(__VA_ARGS__)

class HeapFragmentationManager : public SamplerDescriptorProvider
{
  using BaseType = SamplerDescriptorProvider;

  enum class ResourceMoveResolution
  {
    // Move of the resource has been issued
    MOVING,
    MOVED,
    // Resource has been deleted (standby resource or unused temp / ring memory)
    DELETED,
    // Resource either is already queued for deletion or a currently in use resource will be deleted soon
    QUEUED_FOR_DELETION,
    NO_SPACE,
    // For some reason the resource has to stay at that location (bottom RT AS for example)
    STAYING,
  };

  void onMoveStandbyBufferEntriesSuccess(const dag::Vector<Heap> &buffer_heaps,
    BufferHeapStateWrapper::AccessToken &buffer_heap_state_access, BufferGlobalId old_buffer, BufferGlobalId moved_buffer,
    HeapID heap_id, bool is_emergency_defragmentation);
  void moveBufferDeviceObject(GenericBufferInterface *buffer_object, BufferGlobalId moved_buffer,
    frontend::BindlessManager &bindless_manager, DeviceContext &ctx, ID3D12Device *device,
    BufferHeapStateWrapper::AccessToken &buffer_heap_state_access, bool is_emergency_defragmentation);
  void findBufferOwnersAndReplaceDeviceObjects(BufferGlobalId old_buffer, BufferGlobalId moved_buffer,
    frontend::BindlessManager &bindless_manager, DeviceContext &ctx, ID3D12Device *device,
    BufferHeapStateWrapper::AccessToken &buffer_heap_state_access, bool is_emergency_defragmentation);

  ResourceMoveResolution moveResourceAway(DeviceContext &, DXGIAdapter *, ID3D12Device *, frontend::BindlessManager &, HeapID,
    eastl::monostate, AllocationFlags, bool)
  {
    return ResourceMoveResolution::STAYING;
  }

  ResourceMoveResolution moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
    frontend::BindlessManager &bindless_manager, HeapID heap_id, Image *texture, AllocationFlags allocation_flags,
    bool is_emergency_defragmentation);
  ResourceMoveResolution moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
    frontend::BindlessManager &bindless_manager, HeapID heap_id, BufferGlobalId buffer_id, AllocationFlags allocation_flags,
    bool is_emergency_defragmentation);
  ResourceMoveResolution moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
    frontend::BindlessManager &bindless_manager, HeapID heap_id, AliasHeapReference ref, AllocationFlags allocation_flags,
    bool is_emergency_defragmentation);
  ResourceMoveResolution moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
    frontend::BindlessManager &bindless_manager, HeapID heap_id, ScratchBufferReference ref, AllocationFlags allocation_flags,
    bool is_emergency_defragmentation);
  ResourceMoveResolution moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
    frontend::BindlessManager &bindless_manager, HeapID heap_id, PushRingBufferReference ref, AllocationFlags allocation_flags,
    bool is_emergency_defragmentation);
  ResourceMoveResolution moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
    frontend::BindlessManager &bindless_manager, HeapID heap_id, UploadRingBufferReference ref, AllocationFlags allocation_flags,
    bool is_emergency_defragmentation);
  ResourceMoveResolution moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
    frontend::BindlessManager &bindless_manager, HeapID heap_id, TempUploadBufferReference ref, AllocationFlags allocation_flags,
    bool is_emergency_defragmentation);
  ResourceMoveResolution moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
    frontend::BindlessManager &bindless_manager, HeapID heap_id, PersistentUploadBufferReference ref, AllocationFlags allocation_flags,
    bool is_emergency_defragmentation);
  ResourceMoveResolution moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
    frontend::BindlessManager &bindless_manager, HeapID heap_id, PersistentReadBackBufferReference ref,
    AllocationFlags allocation_flags, bool is_emergency_defragmentation);
  ResourceMoveResolution moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
    frontend::BindlessManager &bindless_manager, HeapID heap_id, PersistentBidirectionalBufferReference ref,
    AllocationFlags allocation_flags, bool is_emergency_defragmentation);
  ResourceMoveResolution moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
    frontend::BindlessManager &bindless_manager, HeapID heap_id, RaytraceAccelerationStructureHeapReference ref,
    AllocationFlags allocation_flags, bool is_emergency_defragmentation);

  bool processGroup(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device, frontend::BindlessManager &bindless_manager,
    uint32_t index);

  bool tryDefragmentHeap(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device, frontend::BindlessManager &bindless_manager,
    uint32_t index, bool is_emergency_defragmentation);

  using HeapIterator = dag::Vector<ResourceHeap>::iterator;
  using RangeIterator = ResourceHeap::FreeRangeSetType::iterator;

  static eastl::pair<HeapIterator, RangeIterator> find_heap_for_defragmentation(dag::Vector<ResourceHeap> &heaps);

  void lockRangesBeforeDefragmentationStep(uint32_t group_index);
  void unlockRangesAfterDefragmentationStep(uint32_t group_index);

  bool processDefragmentationStep(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
    frontend::BindlessManager &bindless_manager, HeapID srcId, ValueRange<uint64_t> selectedRange, bool is_emergency_defragmentation);

  uint32_t groupOffset = 0;

  struct ResourceHeapGroupFragmentationState
  {
    static constexpr uint32_t MAX_LOCKED_RANGES = 8;
    using LockedRangesTable =
      eastl::fixed_vector<eastl::pair<uint32_t, ResourceHeap::FreeRangeSetType::value_type>, MAX_LOCKED_RANGES, false>;

    uint32_t activeMoves = 0;
    LockedRangesTable lockedRanges;
    uint32_t currentGeneration = eastl::numeric_limits<uint32_t>::max();
    uint32_t skipGeneration = eastl::numeric_limits<uint32_t>::max();
  };

  ResourceHeapGroupFragmentationState groupsFragmentationState[ResourceHeapProperties::group_count]{};

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    uint32_t executedMoves[ResourceHeapProperties::group_count]{};
  };

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    for (uint32_t i = 0; i < ResourceHeapProperties::group_count; ++i)
    {
      uint32_t count = data.executedMoves[i];
      data.executedMoves[i] = 0;
      if (groupsFragmentationState[i].activeMoves >= count)
      {
        groupsFragmentationState[i].activeMoves -= count;
      }
      else
      {
        groupsFragmentationState[i].activeMoves = 0;
      }
    }

    BaseType::completeFrameExecution(info, data);
  }

  void completeFrameRecording(Device &device, DXGIAdapter *adapter, frontend::BindlessManager &bindless_manager,
    uint32_t history_index);

  void processEmergencyDefragmentationForGroup(Device &device, DXGIAdapter *adapter, frontend::BindlessManager &bindless_manager,
    uint32_t group_index);

public:
  void processEmergencyDefragmentation(Device &device, DXGIAdapter *adapter, frontend::BindlessManager &bindless_manager,
    uint32_t group_index, bool is_alternate_heap_allowed, bool is_uav, bool is_rtv);
};

class FrameFinalizer : public HeapFragmentationManager
{
  using BaseType = HeapFragmentationManager;
  PendingForCompletedFrameData finalizerData[FRAME_FRAME_BACKLOG_LENGTH] DAG_TS_GUARDED_BY(recodingPendingFrameCompletionMutex){};

protected:
  template <typename T>
  void visitAllFrameFinilazierData(T &visitor)
  {
    OSSpinlockScopedLock lock{recodingPendingFrameCompletionMutex};
    for (auto &f : finalizerData)
    {
      visitor(f);
    }
  }
  void setup(const SetupInfo &info)
  {
    // ensure we never try to access a null pointer
    setRecodingPendingFrameCompletion(&finalizerData[0]);

    BaseType::setup(info);
  }

  void shutdown(DXGIAdapter *adapter, frontend::BindlessManager *bindless_manager)
  {
    // On shutdown we run over all finalizer data to be sure everything has been cleaned up
    CompletedFrameExecutionInfo info;
    info.progressIndex = 0xFFFFFFFFFFFFFFFFull;
    info.historyIndex = 0;
    info.bindlessManager = bindless_manager;
#if _TARGET_PC_WIN
    info.adapter = adapter;
#else
    G_UNUSED(adapter);
#endif
    // make thread sanitizer happy
    {
      OSSpinlockScopedLock lock{recodingPendingFrameCompletionMutex};
      for (; info.historyIndex < countof(finalizerData); ++info.historyIndex)
      {
        BaseType::completeFrameExecution(info, finalizerData[info.historyIndex]);
      }
    }
    BaseType::shutdown();

    setRecodingPendingFrameCompletion(&finalizerData[0]);
  }

  void preRecovery(DXGIAdapter *adapter, frontend::BindlessManager *bindless_manager)
  {
    // Also here we run over all data to be sure its properly cleaned up
    CompletedFrameExecutionInfo info;
    info.progressIndex = 0xFFFFFFFFFFFFFFFFull;
    info.historyIndex = 0;
    info.bindlessManager = bindless_manager;
#if _TARGET_PC_WIN
    info.adapter = adapter;
#else
    G_UNUSED(adapter);
#endif
    // make thread sanitizer happy
    {
      OSSpinlockScopedLock lock{recodingPendingFrameCompletionMutex};
      for (; info.historyIndex < countof(finalizerData); ++info.historyIndex)
      {
        BaseType::completeFrameExecution(info, finalizerData[info.historyIndex]);
      }
    }
    BaseType::preRecovery();

    setRecodingPendingFrameCompletion(&finalizerData[0]);
  }

  struct BeginFrameRecordingInfo
  {
    uint32_t historyIndex;
  };

  void completeFrameExecution(const CompletedFrameExecutionInfo &info)
  {
    // TODO mutex may not be necessary
    OSSpinlockScopedLock lock{recodingPendingFrameCompletionMutex};
    BaseType::completeFrameExecution(info, finalizerData[info.historyIndex]);
  }

  void beginFrameRecording(const BeginFrameRecordingInfo &info)
  {
    setRecodingPendingFrameCompletion(&finalizerData[info.historyIndex]);
  }

  template <typename T>
  void visitFrameCompletionData(T clb)
  {
    OSSpinlockScopedLock lock{recodingPendingFrameCompletionMutex};
    clb.beginVisit();
    for (uint32_t i = 0; i < countof(finalizerData); ++i)
    {
      clb.visitFrameCompletionData(i, countof(finalizerData), finalizerData[i]);
    }
    clb.endVisit();
  }
};

#if DAGOR_DBGLEVEL > 0
class DebugViewBase : public FrameFinalizer
{
  using BaseType = FrameFinalizer;

protected:
  enum class StatusFlag
  {
    PIN_MIN_EVENT_FRAME_RANGE_TO_MAX,
    PIN_MAX_EVENT_FRAME_RANGE_TO_MAX,
    LOAD_VIEWS_FROM_CONFIG,
    STORE_VIEWS_TO_CONFIG,
    COUNT
  };
  using StatusFlags = TypedBitSet<StatusFlag>;

private:
  struct MetricsVisualizerState
  {
    GraphDisplayInfo graphDisplayInfos[static_cast<uint32_t>(Graph::COUNT)];
    ValueRange<uint64_t> shownFrames{0, ~uint64_t(0)};
    StatusFlags statusFlags;
    MetricBits shownBits{};
    char eventObjectNameFilter[MAX_OBJECT_NAME_LENGTH]{};
  };
  MetricsVisualizerState metricsVisualizerState;

protected:
  void setup(const SetupInfo &setup)
  {
    metricsVisualizerState.statusFlags.set(StatusFlag::PIN_MAX_EVENT_FRAME_RANGE_TO_MAX);
    metricsVisualizerState.statusFlags.set(StatusFlag::LOAD_VIEWS_FROM_CONFIG);

    BaseType::setup(setup);
  }

  char *getEventObjectNameFilterBasePointer() { return metricsVisualizerState.eventObjectNameFilter; }

  size_t getEventObjectNameFilterMaxLength() { return countof(metricsVisualizerState.eventObjectNameFilter); }

  bool checkStatusFlag(StatusFlag flag) const { return metricsVisualizerState.statusFlags.test(flag); }

  void setStatusFlag(StatusFlag flag, bool value) { metricsVisualizerState.statusFlags.set(flag, value); }

  const GraphDisplayInfo &getGraphDisplayInfo(Graph graph)
  {
    return metricsVisualizerState.graphDisplayInfos[static_cast<uint32_t>(graph)];
  }

  void setGraphDisplayInfo(Graph graph, const GraphDisplayInfo &info)
  {
    auto &target = metricsVisualizerState.graphDisplayInfos[static_cast<uint32_t>(graph)];
    if (target != info)
    {
      setStatusFlag(StatusFlag::STORE_VIEWS_TO_CONFIG, true);
      target = info;
    }
  }

  template <typename T>
  void iterateGraphDisplayInfos(T clb)
  {
    for (size_t i = 0; i < countof(metricsVisualizerState.graphDisplayInfos); ++i)
    {
      clb(static_cast<Graph>(i), metricsVisualizerState.graphDisplayInfos[i]);
    }
  }

  ValueRange<uint64_t> getShownMetricFrameRange() const { return metricsVisualizerState.shownFrames; }

  void setShownMetricFrameRange(ValueRange<uint64_t> range) { metricsVisualizerState.shownFrames = range; }

  bool isShownMetric(Metric metric) const { return metricsVisualizerState.shownBits.test(metric); }

  void setShownMetric(Metric metric, bool value) { metricsVisualizerState.shownBits.set(metric, value); }

  void storeViewSettings();
  void restoreViewSettings();
};

#if DX12_SUPPORT_RESOURCE_MEMORY_METRICS
class MetricsVisualizer : public DebugViewBase
{
  using BaseType = DebugViewBase;

private:
  struct PlotData
  {
    ConcurrentMetricsStateAccessToken &self;
    ValueRange<uint64_t> viewRange;
    uint64_t selectedFrameIndex;

    const PerFrameData &getCurrentFrame() const { return self->getLastFrameMetrics(); }

    const PerFrameData &getFrame(int idx) const { return self->getFrameMetrics(viewRange.front() + idx); }

    const PerFrameData &getSelectedFrame() const { return self->getFrameMetrics(selectedFrameIndex); }

    bool selectFrame(uint64_t index)
    {
      const auto &baseFrame = self->getFrameMetrics(0);
      selectedFrameIndex = index - baseFrame.frameIndex;
      return selectedFrameIndex < self->getMetricsHistoryLength();
    }

    bool hasAnyData() const { return viewRange.size() > 0; }

    uint64_t getViewSize() const { return viewRange.size(); }
    void resetViewRange(uint64_t from, uint64_t to) { viewRange.reset(from, to); }
    uint64_t getFirstViewIndex() const { return viewRange.front(); }
  };

  template <typename T, typename U>
  static T getPlotPointFrameValue(int idx, void *data)
  {
    auto info = reinterpret_cast<PlotData *>(data);
    const auto &frame = info->getFrame(idx);
    return //
      {double(frame.frameIndex), double(U::get(frame))};
  }

  template <typename T>
  static T getPlotPointFrameBase(int idx, void *data)
  {
    struct GetZero
    {
      static double get(const PerFrameData &) { return 0; }
    };
    return getPlotPointFrameValue<T, GetZero>(idx, data);
  }

  void drawPersistentBufferBasicMetricsTable(const char *id, const CounterWithSizeInfo &usage_counter,
    const CounterWithSizeInfo &buffer_counter, const PeakCounterWithSizeInfo &peak_memory_counter,
    const PeakCounterWithSizeInfo &peak_buffer_counter);
  void drawRingMemoryBasicMetricsTable(const char *id, const CounterWithSizeInfo &current_counter,
    const CounterWithSizeInfo &summary_counter, const PeakCounterWithSizeInfo &peak_counter,
    const PeakCounterWithSizeInfo &peak_buffer_counter);

  void drawBufferMemoryEvent(const MetricsState::ActionInfo &event, uint64_t index);
  void drawTextureEvent(const MetricsState::ActionInfo &event, uint64_t index);
  void drawHostSharedMemoryEvent(const MetricsState::ActionInfo &event, uint64_t index);
  bool drawEvent(const MetricsState::ActionInfo &event, uint64_t index);
  GraphDisplayInfo drawGraphViewControls(Graph graph);
  void setupPlotY2CountRange(uint64_t max_value, bool has_data);
  void setupPlotYMemoryRange(uint64_t max_value, bool has_data);
  PlotData setupPlotXRange(ConcurrentMetricsStateAccessToken &access, const GraphDisplayInfo &display_state, bool has_data);

protected:
  void drawMetricsCaptureControls();
  void drawMetricsEventsView();
  void drawMetricsEvnetsViewFilterControls();
  void drawSystemCurrentMemoryUseTable();
  void drawSystemMemoryUsePlot();
  void drawHeapsPlot();
  void drawHeapsSummaryTable();
  void drawScratchBufferTable();
  void drawScratchBufferPlot();
  void drawTexturePlot();
  void drawBuffersPlot();
  void drawConstRingMemoryPlot();
  void drawConstRingBasicMetricsTable();
  void drawUploadRingMemoryPlot();
  void drawUploadRingBasicMetricsTable();
  void drawTempuraryUploadMemoryPlot();
  void drawTemporaryUploadMemoryBasicMetrics();
  void drawRaytracePlot();
  void drawUserHeapsPlot();
  void drawPersistenUploadMemoryMetricsTable();
  void drawPersistenReadBackMemoryMetricsTable();
  void drawPersistenBidirectioanlMemoryMetricsTable();
  void drawPersistenUploadMemoryPlot();
  void drawPersistenReadBackMemoryPlot();
  void drawPersistenBidirectioanlMemoryPlot();
  void drawRaytraceSummaryTable();
};
#else
class MetricsVisualizer : public DebugViewBase
{
  using BaseType = DebugViewBase;

protected:
  void drawMetricsCaptureControls();
  void drawMetricsEventsView();
  void drawMetricsEvnetsViewFilterControls();
  void drawSystemCurrentMemoryUseTable();
  void drawSystemMemoryUsePlot();
  void drawHeapsPlot();
  void drawHeapsSummaryTable();
  void drawScratchBufferTable();
  void drawScratchBufferPlot();
  void drawTexturePlot();
  void drawBuffersPlot();
  void drawConstRingMemoryPlot();
  void drawConstRingBasicMetricsTable();
  void drawUploadRingMemoryPlot();
  void drawUploadRingBasicMetricsTable();
  void drawTempuraryUploadMemoryPlot();
  void drawTemporaryUploadMemoryBasicMetrics();
  void drawRaytracePlot();
  void drawUserHeapsPlot();
  void drawPersistenUploadMemoryMetricsTable();
  void drawPersistenReadBackMemoryMetricsTable();
  void drawPersistenBidirectioanlMemoryMetricsTable();
  void drawPersistenUploadMemoryPlot();
  void drawPersistenReadBackMemoryPlot();
  void drawPersistenBidirectioanlMemoryPlot();
  void drawRaytraceSummaryTable();
};
#endif

class DebugView : public MetricsVisualizer
{
  using BaseType = MetricsVisualizer;

  void drawPersistenUploadMemorySegmentTable();
  void drawPersistenReadBackMemorySegmentTable();
  void drawPersistenBidirectioanlMemorySegmentTable();
  void drawPersistenUploadMemoryInfoView();
  void drawPersistenReadBackMemoryInfoView();
  void drawPersistenBidirectioanlMemoryInfoView();
  void drawUserHeapsTable();
  void drawUserHeapsSummaryTable();
  void drawRaytraceTopStructureTable();
  void drawRaytraceBottomStructureTable();
  void drawTempuraryUploadMemorySegmentsTable();
  void drawUploadRingSegmentsTable();
  void drawConstRingSegmentsTable();
  void drawTextureTable();
  void drawBuffersTable();
  void drawRaytracingInfoView();
#if DX12_USE_ESRAM
  void drawESRamInfoView();
#endif
  void drawTemporaryUploadMemoryInfoView();
  void drawUploadRingBufferInfoView();
  void drawConstRingBufferInfoView();
  void drawBuffersInfoView();
  void drawTexturesInfoView();
  void drawScratchBufferInfoView();
  void drawUserHeapInfoView();
  void drawHeapsTable();
  void drawHeapInfoView();
  void drawSystemInfoView();
  void drawManagerBehaviorInfoView();
  void drawEventsInfoView();
  void drawMetricsInfoView();
  void drawSamplersInfoView();
  void drawSamplerTable();

public:
  void debugOverlay();
};
#else
using DebugView = FrameFinalizer;
#endif
} // namespace resource_manager

class ResourceMemoryHeap : private resource_manager::DebugView
{
  using BaseType = resource_manager::DebugView;

public:
  ResourceMemoryHeap() = default;
  ~ResourceMemoryHeap() = default;
  ResourceMemoryHeap(const ResourceMemoryHeap &) = delete;
  ResourceMemoryHeap &operator=(const ResourceMemoryHeap &) = delete;
  ResourceMemoryHeap(ResourceMemoryHeap &&) = delete;
  ResourceMemoryHeap &operator=(ResourceMemoryHeap &&) = delete;

  using Metric = BaseType::Metric;
  using FreeReason = BaseType::FreeReason;
  using CompletedFrameExecutionInfo = BaseType::CompletedFrameExecutionInfo;
  using CompletedFrameRecordingInfo = BaseType::CompletedFrameRecordingInfo;
  using SetupInfo = BaseType::SetupInfo;
  using BeginFrameRecordingInfo = BaseType::BeginFrameRecordingInfo;

  using BaseType::preRecovery;
  using BaseType::setup;
  using BaseType::shutdown;

  using BaseType::completeFrameExecution;

  using BaseType::beginFrameRecording;

#if D3D_HAS_RAY_TRACING
  using BaseType::deleteRaytraceBottomAccelerationStructureOnFrameCompletion;
  using BaseType::deleteRaytraceTopAccelerationStructureOnFrameCompletion;
  using BaseType::getRaytraceAccelerationStructuresGpuMemoryUsage;
  using BaseType::newRaytraceBottomAccelerationStructure;
  using BaseType::newRaytraceTopAccelerationStructure;
#endif

  using BaseType::getSwapchainColorGlobalId;
  using BaseType::getSwapchainSecondaryColorGlobalId;

#if DAGOR_DBGLEVEL > 0
  using BaseType::debugOverlay;
#endif

  using BaseType::freeUserHeapOnFrameCompletion;
  using BaseType::getResourceHeapGroupProperties;
  using BaseType::getUserHeapMemory;
  using BaseType::newUserHeap;
  using BaseType::placeBufferInHeap;
  using BaseType::placeTextureInHeap;
  using BaseType::visitAliasingHeaps;
  using BaseType::visitHeaps;

  using BaseType::getResourceAllocationProperties;

  using BaseType::getFramePushRingMemorySize;
  using BaseType::getPersistentBidirectionalMemorySize;
  using BaseType::getPersistentReadBackMemorySize;
  using BaseType::getPersistentUploadMemorySize;
  using BaseType::getTemporaryUploadMemorySize;
  using BaseType::getUploadRingMemorySize;

  using BaseType::getActiveTextureObjectCount;
  using BaseType::getTextureObjectCapacity;
  using BaseType::reserveTextureObjects;

  using BaseType::getActiveBufferObjectCount;
  using BaseType::getBufferObjectCapacity;
  using BaseType::reserveBufferObjects;

  using BaseType::deleteBufferObject;
  using BaseType::getResourceMemoryForBuffer;

  using BaseType::allocatePersistentBidirectional;
  using BaseType::allocatePersistentReadBack;
  using BaseType::allocatePersistentUploadMemory;
  using BaseType::allocatePushMemory;
  using BaseType::allocateTempUpload;
  using BaseType::allocateTempUploadForUploadBuffer;
  using BaseType::allocateUploadRingMemory;
  using BaseType::freeHostDeviceSharedMemoryRegionForUploadBufferOnFrameCompletion;
  using BaseType::freeHostDeviceSharedMemoryRegionOnFrameCompletion;

  using BaseType::getTempScratchBufferSpace;

  using BaseType::allocateTextureDSVDescriptor;
  using BaseType::allocateTextureRTVDescriptor;
  using BaseType::allocateTextureSRVDescriptor;
  using BaseType::freeTextureDSVDescriptor;
  using BaseType::freeTextureRTVDescriptor;
  using BaseType::freeTextureSRVDescriptor;

  using BaseType::allocateBufferSRVDescriptor;

  using BaseType::adoptTexture;
  using BaseType::aliasTexture;
  using BaseType::createTexture;
  using BaseType::deleteTextureObjectOnFrameCompletion;
  using BaseType::destroyTextureOnFrameCompletion;
  using BaseType::newTextureObject;
  using BaseType::PendingForCompletedFrameData;
  using BaseType::visitFrameCompletionData;
  using BaseType::visitImageObjects;
  using BaseType::visitImageObjectsExplicit;
  using BaseType::visitTextureObjects;
  using BaseType::visitTextureObjectsExplicit;

  using BaseType::allocateBuffer;
  using BaseType::createBufferRawSRV;
  using BaseType::createBufferRawUAV;
  using BaseType::createBufferStructureSRV;
  using BaseType::createBufferStructureUAV;
  using BaseType::createBufferTextureSRV;
  using BaseType::createBufferTextureUAV;
  using BaseType::discardBuffer;
  using BaseType::freeBufferOnFrameCompletion;
  using BaseType::newBufferObject;
  using BaseType::visitBufferObjects;
  using BaseType::visitBufferObjectsExplicit;
  using BaseType::visitBuffers;

  using BaseType::getDeviceLocalAvailablePoolBudget;
  using BaseType::getDeviceLocalBudget;
#if _TARGET_PC_WIN
  using BaseType::getDeviceLocalPhysicalLimit;
#endif
  using BaseType::getHostLocalAvailablePoolBudget;
  using BaseType::getHostLocalBudget;
  using BaseType::isUMASystem;

  using BaseType::setTempBufferShrinkThresholdSize;

  using BaseType::popEvent;
  using BaseType::pushEvent;

  using BaseType::completeFrameRecording;

  using BaseType::processEmergencyDefragmentation;

#if DX12_USE_ESRAM
  using BaseType::aliasESRamTexture;
  using BaseType::createESRamTexture;
  using BaseType::deselectLayout;
  using BaseType::fetchMovableTextures;
  using BaseType::hasESRam;
  using BaseType::registerMovableTexture;
  using BaseType::resetAllLayouts;
  using BaseType::selectLayout;
  using BaseType::unmapResource;
  using BaseType::writeBackMovableTextures;
#endif

  using BaseType::enterSuspended;
  using BaseType::leaveSuspended;

  void waitForTemporaryUsesToFinish()
  {
    uint32_t retries = 0;
    logdbg("DX12: Begin waiting for temporary memory uses to be completed...");
    while (!allTemporaryUsesCompleted([this](auto invoked) {
      logdbg("DX12: Checking for temporary memory uses to be completed...");
      visitAllFrameFinilazierData(invoked);
    }))
    {
      if (++retries > 100)
      {
        logdbg("DX12: Waiting for too long, exiting, risking crashes...");
        break;
      }
      logdbg("DX12: Wating 10ms to complete temporary memory uses...");
      sleep_msec(10);
    }
    logdbg("DX12: Finished waiting for temporary memory uses...");
  }
  void generateResourceAndMemoryReport(DXGIAdapter *adapter, uint32_t *num_textures, uint64_t *total_mem, String *out_text);
  void fillResourceDump(Tab<ResourceDumpInfo> &dump_container);

  using BaseType::AliasHeapReference;
  using BaseType::AnyResourceReference;
  using BaseType::PersistentBidirectionalBufferReference;
  using BaseType::PersistentReadBackBufferReference;
  using BaseType::PersistentUploadBufferReference;
  using BaseType::PushRingBufferReference;
  using BaseType::RaytraceAccelerationStructureHeapReference;
  using BaseType::reportOOMInformation;
  using BaseType::ScratchBufferReference;
  using BaseType::TempUploadBufferReference;
  using BaseType::UploadRingBufferReference;

  using BaseType::isImageAlive;

  using BaseType::createSampler;
  using BaseType::getSampler;
  using BaseType::getSamplerInfo;
};
} // namespace drv3d_dx12
