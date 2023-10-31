#pragma once
#include <drv_returnAddrStore.h>
#include "query_manager.h"
#include <EASTL/variant.h>
#include <osApiWrappers/dag_events.h>
#include <emmintrin.h>
#include <3d/dag_drvDecl.h>
#include <osApiWrappers/dag_threads.h>
#include <3d/dag_drv3dConsts.h>
#include <3d/tql.h>
#include "variant_vector.h"
#include "texture.h"
#include "buffer.h"
#include "resource_memory_heap.h"
#include "bindless.h"
#include "ngx_wrapper.h"
#include "xess_wrapper.h"
#include "fsr2_wrapper.h"

#include "debug/device_context_state.h"
// #include "render_graph.h"

#include "command_list.h"
#include "stateful_command_buffer.h"
#include "resource_state_tracker.h"

namespace drv3d_dx12
{
template <typename T, size_t N>
class ForwardRing
{
  T elements[N]{};
  T *selectedElement = &elements[0];

  ptrdiff_t getIndex(const T *pos) const { return pos - &elements[0]; }

  T *advancePosition(T *pos) { return &elements[(getIndex(pos) + 1) % N]; }

public:
  T &get() { return *selectedElement; }
  const T &get() const { return *selectedElement; }
  T &operator*() { return *selectedElement; }
  const T &operator*() const { return *selectedElement; }
  T *operator->() { return selectedElement; }
  const T *operator->() const { return selectedElement; }

  constexpr size_t size() const { return N; }

  void advance() { selectedElement = advancePosition(selectedElement); }

  // Iterates all elements of the ring in a unspecified order
  template <typename T>
  void iterate(T clb)
  {
    for (auto &&v : elements)
    {
      clb(v);
    }
  }

  // Iterates all elements from last to the current element
  template <typename T>
  void walkAll(T clb)
  {
    auto at = advancePosition(selectedElement);
    for (size_t i = 0; i < N; ++i)
    {
      clb(*at);
      at = advancePosition(at);
    }
  }

  // Iterates all elements from last to the element before the current element
  template <typename T>
  void walkUnitlCurrent(T clb)
  {
    auto at = advancePosition(selectedElement);
    for (size_t i = 0; i < N - 1; ++i)
    {
      clb(at);
      at = advancePosition(at);
    }
  }
};
// Ring buffer acting as a thread safe, non blocking, single producer, single consumer pipe
template <typename T, size_t N>
class ConcurrentRingPipe
{
  T pipe[N]{};
  alignas(64) std::atomic<size_t> readPos{0};
  alignas(64) std::atomic<size_t> writePos{0};

public:
  T *tryAcquireRead()
  {
    auto rp = readPos.load(std::memory_order_relaxed);
    if (rp < writePos.load(std::memory_order_acquire))
    {
      return &pipe[rp % N];
    }
    return nullptr;
  }
  void releaseRead() { ++readPos; }
  T *tryAcquireWrite()
  {
    auto wp = writePos.load(std::memory_order_relaxed);
    if (wp - readPos.load(std::memory_order_acquire) < N)
      return &pipe[wp % N];
    return nullptr;
  }
  void releaseWrite() { ++writePos; }
  bool canAcquireWrite() const { return (writePos.load(std::memory_order_relaxed) - readPos.load(std::memory_order_acquire)) < N; }
};

// Specialization with only one entry, this is used for builds that only support
// concurrent execution mode and don't need more than one pipe element.
// No call will block, return null or fail.
template <typename T>
class ConcurrentRingPipe<T, 1>
{
  T pipe{};

public:
  T *tryAcquireRead() { return &pipe; }
  void releaseRead() {}
  T *tryAcquireWrite() { return &pipe; }
  void releaseWrite() {}
  constexpr bool canAcquireWrite() const { return true; }
};

// Extended ConcurrentRingPipe that implements blocking read/write
template <typename T, size_t N>
class WaitableConcurrentRingPipe : private ConcurrentRingPipe<T, N>
{
  using BaseType = ConcurrentRingPipe<T, N>;

  os_event_t itemsToRead;
  os_event_t itemsToWrite;

public:
  WaitableConcurrentRingPipe()
  {
    os_event_create(&itemsToRead, "WCRP:itemsToRead");
    os_event_create(&itemsToWrite, "WCRP:itemsToWrite");
  }
  ~WaitableConcurrentRingPipe()
  {
    os_event_destroy(&itemsToRead);
    os_event_destroy(&itemsToWrite);
  }
  using BaseType::tryAcquireRead;
  void releaseRead()
  {
    BaseType::releaseRead();
    os_event_set(&itemsToWrite);
  }
  using BaseType::tryAcquireWrite;
  void releaseWrite()
  {
    BaseType::releaseWrite();
    os_event_set(&itemsToRead);
  }
  T &acquireRead()
  {
    T *result = nullptr;
    try_and_wait_with_os_event(itemsToRead, DEFAULT_WAIT_SPINS,
      [this, &result]() //
      {
        result = tryAcquireRead();
        return result != nullptr;
      });
    return *result;
  }
  // repeatedly call clb until it either returns false or a T could be acquired
  // return value of acquireRead can be nullptr if clb returned false
  template <typename U>
  T *acquireRead(U &&clb)
  {
    T *result = nullptr;
    try_and_wait_with_os_event(itemsToRead, DEFAULT_WAIT_SPINS,
      [this, &result, &clb]() //
      {
        if (!clb())
          return true;
        result = tryAcquireRead();
        return result != nullptr;
      });
    return result;
  }
  T &acquireWrite()
  {
    T *result = nullptr;
    try_and_wait_with_os_event(itemsToWrite, DEFAULT_WAIT_SPINS,
      [this, &result]() //
      {
        result = tryAcquireWrite();
        return result != nullptr;
      });
    return *result;
  }
  // repeatedly call clb until it either returns false or a T could be acquired
  // return value of acquireRead can be nullptr if clb returned false
  template <typename U>
  T *acquireWrite(U &&clb)
  {
    T *result = nullptr;
    try_and_wait_with_os_event(itemsToWrite, DEFAULT_WAIT_SPINS,
      [this, &result, &clb]() //
      {
        if (!clb())
          return true;
        result = tryAcquireWrite();
        return result != nullptr;
      });
    return result;
  }
  using BaseType::canAcquireWrite;
  void waitForWriteItem()
  {
    try_and_wait_with_os_event(itemsToWrite, DEFAULT_WAIT_SPINS, [this]() { return this->canAcquireWrite(); });
  }
};


// Specialization with only one entry, this is used for builds that only support
// concurrent execution mode and don't need more than one pipe element.
// No call will block, return null or fail.
template <typename T>
class WaitableConcurrentRingPipe<T, 1> : private ConcurrentRingPipe<T, 1>
{
  using BaseType = ConcurrentRingPipe<T, 1>;

public:
  using BaseType::releaseRead;
  using BaseType::releaseWrite;
  using BaseType::tryAcquireRead;
  using BaseType::tryAcquireWrite;
  T &acquireRead() { return *tryAcquireRead(); } // -V522
  // repeatedly call clb until it either returns false or a T could be acquired
  // return value of acquireRead can be nullptr if clb returned false
  template <typename U>
  T *acquireRead(U &&)
  {
    return acquireRead();
  }
  T &acquireWrite() { return *tryAcquireWrite(); } // -V522
  // repeatedly call clb until it either returns false or a T could be acquired
  // return value of acquireRead can be nullptr if clb returned false
  template <typename U>
  T *acquireWrite(U &&)
  {
    return acquireWrite();
  }
  void waitForWriteItem() {}
  using BaseType::canAcquireWrite;
};
#if D3D_HAS_RAY_TRACING
struct RaytraceGeometryDescriptionBufferResourceReferenceSet
{
  BufferResourceReference vertexOrAABBBuffer;
  BufferResourceReference indexBuffer;
  BufferResourceReference transformBuffer;
};
#endif

constexpr uint32_t timing_history_length = 32;

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

struct ImageSubresourceRange
{
  MipMapRange mipMapRange;
  ArrayLayerRange arrayLayerRange;
};
struct ClearDepthStencilValue
{
  float depth;
  uint32_t stencil;
};
union ClearColorValue
{
  float float32[4];
  int32_t int32[4];
  uint32_t uint32[4];
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
struct ImageCopy
{
  SubresourceIndex srcSubresource;
  SubresourceIndex dstSubresource;
  Offset3D dstOffset;
  D3D12_BOX srcBox;
};
inline ImageCopy make_whole_resource_copy_info()
{
  ImageCopy result{};
  result.srcSubresource = SubresourceIndex::make(~uint32_t{0});
  return result;
}
inline bool is_whole_resource_copy_info(const ImageCopy &copy) { return copy.srcSubresource == SubresourceIndex::make(~uint32_t{0}); }

union FramebufferMask
{
  uint32_t raw = 0;
  struct
  {
    uint32_t hasDepthStencilAttachment : 1;
    uint32_t isConstantDepthstencilAttachment : 1;
    uint32_t colorAttachmentMask : Driver3dRenderTarget::MAX_SIMRT;
  };

  template <typename T>
  void iterateColorAttachments(T &&u)
  {
    for_each_set_bit(colorAttachmentMask, eastl::forward<T>(u));
  }

  template <typename T>
  void iterateColorAttachments(uint32_t valid_mask, T &&u)
  {
    for_each_set_bit(colorAttachmentMask & valid_mask, eastl::forward<T>(u));
  }

  void setColorAttachment(uint32_t index) { colorAttachmentMask |= 1u << index; }

  void resetColorAttachment(uint32_t index) { colorAttachmentMask &= ~(1u << index); }

  void setDepthStencilAttachment(bool is_const)
  {
    hasDepthStencilAttachment = 1;
    isConstantDepthstencilAttachment = is_const ? 1u : 0u;
  }

  void resetDepthStencilAttachment()
  {
    hasDepthStencilAttachment = 0;
    isConstantDepthstencilAttachment = 0;
  }

  void resetAll() { raw = 0; }
};

struct FramebufferInfo
{
  struct AttachmentInfo
  {
    Image *image = nullptr;
    ImageViewState view = {};
  };
  AttachmentInfo colorAttachments[Driver3dRenderTarget::MAX_SIMRT] = {};
  AttachmentInfo depthStencilAttachment = {};
  FramebufferMask attachmentMask;


  FramebufferMask getMatchingAttachmentMask(Image *texture)
  {
    FramebufferMask result;
    for (uint32_t i = 0; i < array_size(colorAttachments); ++i)
    {
      result.colorAttachmentMask |= ((colorAttachments[i].image == texture) ? 1u : 0u) << i;
    }
    result.isConstantDepthstencilAttachment = attachmentMask.isConstantDepthstencilAttachment;
    result.hasDepthStencilAttachment = (depthStencilAttachment.image == texture) ? 1u : 0u;
    return result;
  }

  void setColorAttachment(uint32_t index, Image *image, ImageViewState view)
  {
    auto &target = colorAttachments[index];
    target.image = image;
    target.view = view;
    attachmentMask.setColorAttachment(index);
  }
  void clearColorAttachment(uint32_t index)
  {
    colorAttachments[index] = AttachmentInfo{};
    attachmentMask.resetColorAttachment(index);
  }
  void setDepthStencilAttachment(Image *image, ImageViewState view, bool read_only)
  {
    depthStencilAttachment.image = image;
    depthStencilAttachment.view = view;
    attachmentMask.setDepthStencilAttachment(read_only);
  }
  void clearDepthStencilAttachment()
  {
    depthStencilAttachment = AttachmentInfo{};
    attachmentMask.resetDepthStencilAttachment();
  }
  bool hasConstDepthStencilAttachment() const { return 0 != attachmentMask.isConstantDepthstencilAttachment; }
  bool hasDepthStencilAttachment() const { return 0 != attachmentMask.hasDepthStencilAttachment; }
  Extent2D makeDrawArea(Extent2D def = {}) const;
};


struct FramebufferDescription
{
  D3D12_CPU_DESCRIPTOR_HANDLE colorAttachments[Driver3dRenderTarget::MAX_SIMRT];
  D3D12_CPU_DESCRIPTOR_HANDLE depthStencilAttachment;

  void clear(D3D12_CPU_DESCRIPTOR_HANDLE null_color)
  {
    eastl::fill(eastl::begin(colorAttachments), eastl::end(colorAttachments), null_color);
    depthStencilAttachment.ptr = 0;
  }

  bool contains(D3D12_CPU_DESCRIPTOR_HANDLE view) const
  {
    using namespace eastl;
    return (end(colorAttachments) != find(begin(colorAttachments), end(colorAttachments), view)) || (depthStencilAttachment == view);
  }
  friend bool operator==(const FramebufferDescription &l, const FramebufferDescription &r)
  {
    return 0 == memcmp(&l, &r, sizeof(FramebufferDescription));
  }
  friend bool operator!=(const FramebufferDescription &l, const FramebufferDescription &r) { return !(l == r); }
};

struct FramebufferState
{
  FramebufferInfo frontendFrameBufferInfo = {};
  FramebufferLayout framebufferLayout = {};
  FramebufferDescription frameBufferInfo = {};
  FramebufferMask framebufferDirtyState;

  void dirtyTextureState(Image *texture)
  {
    auto state = frontendFrameBufferInfo.getMatchingAttachmentMask(texture);
    framebufferDirtyState.raw |= state.raw;
  }

  void dirtyAllTexturesState() { framebufferDirtyState = frontendFrameBufferInfo.attachmentMask; }

  void clear(D3D12_CPU_DESCRIPTOR_HANDLE null_color)
  {
    frontendFrameBufferInfo = FramebufferInfo{};
    framebufferLayout.clear();
    frameBufferInfo.clear(null_color);
    framebufferDirtyState.resetAll();
  }

  void bindColorTarget(uint32_t index, Image *image, ImageViewState ivs, D3D12_CPU_DESCRIPTOR_HANDLE view)
  {
    frontendFrameBufferInfo.setColorAttachment(index, image, ivs);
    framebufferLayout.colorTargetMask |= 1 << index;
    framebufferLayout.colorFormats[index] = ivs.getFormat();
    frameBufferInfo.colorAttachments[index] = view;
    framebufferDirtyState.setColorAttachment(index);
  }

  void clearColorTarget(uint32_t index, D3D12_CPU_DESCRIPTOR_HANDLE null_handle)
  {
    frontendFrameBufferInfo.clearColorAttachment(index);
    framebufferLayout.colorTargetMask &= ~(1 << index);
    framebufferLayout.colorFormats[index] = FormatStore(0);
    frameBufferInfo.colorAttachments[index] = null_handle;
    framebufferDirtyState.resetColorAttachment(index);
  }

  void bindDepthStencilTarget(Image *image, ImageViewState ivs, D3D12_CPU_DESCRIPTOR_HANDLE view, bool read_only)
  {
    frontendFrameBufferInfo.setDepthStencilAttachment(image, ivs, read_only);
    framebufferLayout.hasDepth = 1;
    framebufferLayout.depthStencilFormat = ivs.getFormat();
    frameBufferInfo.depthStencilAttachment = view;
    framebufferDirtyState.setDepthStencilAttachment(read_only);
  }

  void clearDepthStencilTarget()
  {
    frontendFrameBufferInfo.clearDepthStencilAttachment();
    framebufferLayout.hasDepth = 0;
    framebufferLayout.depthStencilFormat = FormatStore(0);
    frameBufferInfo.depthStencilAttachment.ptr = 0;
    framebufferDirtyState.resetDepthStencilAttachment();
  }
};

struct GraphicsState
{
  enum
  {
    SCISSOR_RECT_DIRTY,
    SCISSOR_ENABLED,
    SCISSOR_ENABLED_DIRTY,
    VIEWPORT_DIRTY,
    USE_WIREFRAME,

    INDEX_BUFFER_DIRTY,
    VERTEX_BUFFER_0_DIRTY,
    VERTEX_BUFFER_1_DIRTY,
    VERTEX_BUFFER_2_DIRTY,
    VERTEX_BUFFER_3_DIRTY,
    // state
    INDEX_BUFFER_STATE_DIRTY,
    VERTEX_BUFFER_STATE_0_DIRTY,
    VERTEX_BUFFER_STATE_1_DIRTY,
    VERTEX_BUFFER_STATE_2_DIRTY,
    VERTEX_BUFFER_STATE_3_DIRTY,

    PREDICATION_BUFFER_STATE_DIRTY,

    COUNT
  };
  BasePipeline *basePipeline = nullptr;
  PipelineVariant *pipeline = nullptr;
  D3D12_PRIMITIVE_TOPOLOGY_TYPE topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;

  InputLayoutID inputLayoutIdent = InputLayoutID::Null();
  StaticRenderStateID staticRenderStateIdent = StaticRenderStateID::Null();
  FramebufferLayoutID framebufferLayoutID = FramebufferLayoutID::Null();
  FramebufferState framebufferState;
  D3D12_RECT scissorRects[Viewport::MAX_VIEWPORT_COUNT] = {};
  uint32_t scissorRectCount = 0;
  D3D12_VIEWPORT viewports[Viewport::MAX_VIEWPORT_COUNT] = {};
  uint32_t viewportCount = 0;
  eastl::bitset<COUNT> statusBits{0};

  BufferResourceReferenceAndAddressRange indexBuffer;
  DXGI_FORMAT indexBufferFormat = DXGI_FORMAT_R16_UINT;
  BufferResourceReferenceAndAddressRange vertexBuffers[MAX_VERTEX_INPUT_STREAMS];
  uint32_t vertexBufferStrides[MAX_VERTEX_INPUT_STREAMS] = {};
  BufferResourceReferenceAndOffset predicationBuffer;
  BufferResourceReferenceAndOffset activePredicationBuffer;

  void dirtyBufferState(BufferGlobalId ident)
  {
    if (indexBuffer.resourceId == ident)
    {
      statusBits.set(INDEX_BUFFER_STATE_DIRTY);
    }
    for (uint32_t i = 0; i < array_size(vertexBuffers); ++i)
    {
      if (vertexBuffers[i].resourceId == ident)
      {
        statusBits.set(VERTEX_BUFFER_STATE_0_DIRTY + i);
      }
    }
    if (predicationBuffer.resourceId == ident)
    {
      statusBits.set(PREDICATION_BUFFER_STATE_DIRTY);
    }
  }

  void dirtyTextureState(Image *texture) { framebufferState.dirtyTextureState(texture); }

  void onFlush()
  {
    G_STATIC_ASSERT(4 == MAX_VERTEX_INPUT_STREAMS);
    framebufferLayoutID = FramebufferLayoutID::Null();
    statusBits.set(INDEX_BUFFER_DIRTY);
    statusBits.set(INDEX_BUFFER_STATE_DIRTY);
    for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
    {
      statusBits.set(VERTEX_BUFFER_0_DIRTY + i);
      statusBits.set(VERTEX_BUFFER_STATE_0_DIRTY + i);
    }
    statusBits.set(PREDICATION_BUFFER_STATE_DIRTY);
    activePredicationBuffer = {};

    framebufferState.dirtyAllTexturesState();
  }

  void onFrameStateInvalidate(D3D12_CPU_DESCRIPTOR_HANDLE null_ct)
  {
    pipeline = nullptr;

    statusBits.set(GraphicsState::VIEWPORT_DIRTY);
    statusBits.set(GraphicsState::SCISSOR_RECT_DIRTY);

    framebufferState.clear(null_ct);

    basePipeline = nullptr;
    framebufferLayoutID = FramebufferLayoutID::Null();

    indexBuffer = {};
    statusBits.set(INDEX_BUFFER_DIRTY);
    statusBits.set(INDEX_BUFFER_STATE_DIRTY);

    for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
    {
      statusBits.set(VERTEX_BUFFER_0_DIRTY + i);
      statusBits.set(VERTEX_BUFFER_STATE_0_DIRTY + i);
      vertexBuffers[i] = {};
    }

    predicationBuffer = {};
    activePredicationBuffer = {};
    statusBits.set(PREDICATION_BUFFER_STATE_DIRTY);

    framebufferState.dirtyAllTexturesState();
  }
};
struct ComputeState
{
  ComputePipeline *pipeline = nullptr;

  void onFlush() {}
  void onFrameStateInvalidate() { pipeline = nullptr; }
};

class Query;
class Device;
class DeviceContext;
struct DeviceResourceData;
class BackendQueryManager;
#if D3D_HAS_RAY_TRACING
struct RaytraceAccelerationStructure;
#endif


struct DeviceFeaturesConfig
{
  enum Bits
  {
    OPTIMIZE_BUFFER_UPLOADS,
    USE_THREADED_COMMAND_EXECUTION,
    PIPELINE_COMPILATION_ERROR_IS_FATAL,
    ASSERT_ON_PIPELINE_COMPILATION_ERROR,
    ALLOW_OS_MANAGED_SHADER_CACHE,
    DISABLE_PIPELINE_LIBRARY_CACHE,
#if _TARGET_SCARLETT
    REPORT_WAVE_64,
#endif
#if DX12_CONFIGUREABLE_BARRIER_MODE
    PROCESS_USER_BARRIERS,
    VALIDATE_USER_BARRIERS,
    GENERATE_ALL_BARRIERS,
#endif
#if DX12_DOES_SET_DEBUG_NAMES
    NAME_OBJECTS,
#endif
    ALLOW_STREAM_BUFFERS,
    ALLOW_STREAM_CONST_BUFFERS,
    ALLOW_STREAM_VERTEX_BUFFERS,
    ALLOW_STREAM_INDEX_BUFFERS,
    ALLOW_STREAM_INDIRECT_BUFFERS,
    ALLOW_STREAM_STAGING_BUFFERS,
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
    ROOT_SIGNATURES_USES_CBV_DESCRIPTOR_RANGES,
#endif
    DISABLE_BUFFER_SUBALLOCATION,
    DISABLE_BINDLESS,
    IGNORE_PREDICATION,
    COUNT,
    INVLID = COUNT
  };
  typedef eastl::bitset<COUNT> Type;
};
// very simple fifo queue with a fixed amount of slots
template <typename T, size_t N>
class FixedFifoQueue
{
  T store[N] = {};
  size_t top = 0;
  size_t count = 0;

public:
  FixedFifoQueue() = default;
  ~FixedFifoQueue() = default;
  FixedFifoQueue(const FixedFifoQueue &) = default;
  FixedFifoQueue &operator=(const FixedFifoQueue &) = default;
  FixedFifoQueue(FixedFifoQueue &&) = default;
  FixedFifoQueue &operator=(FixedFifoQueue &&) = default;

  template <typename U>
  void push_back(U &&v)
  {
    store[(top + count) % N] = eastl::forward<U>(v);
    ++count;
  }
  void pop_front()
  {
    top = (top + 1) % N;
    --count;
  }
  T &front() { return store[top]; }
  bool empty() const { return 0 == count; }
  bool full() const { return N == count; }

  template <typename U>
  size_t enumerate(U &&clb)
  {
    for (size_t i = 0; i < count; ++i)
      clb(i, store[(top + i) % N]);
    return count;
  }

  template <typename U>
  size_t enumerateReverse(U &&clb)
  {
    for (size_t i = 0; i < count; ++i)
      clb(count - i - 1, store[(top + (count - i - 1)) % N]);
    return count;
  }
};
class Query;

template <typename>
struct GetSmartPointerOfCommandBufferPointer;

template <typename... As>
struct GetSmartPointerOfCommandBufferPointer<VersionedPtr<As...>>
{
  using Type = VersionedComPtr<As...>;
};

template <typename CommandListResultType, D3D12_COMMAND_LIST_TYPE CommandListTypeName,
  typename CommandListStoreType = typename GetSmartPointerOfCommandBufferPointer<CommandListResultType>::Type>
struct CommandStreamSet
{
  ComPtr<ID3D12CommandAllocator> pool;
  eastl::vector<CommandListStoreType> lists;
  uint32_t listsInUse = 0;

  void init(ID3D12Device *device) { DX12_CHECK_RESULT(device->CreateCommandAllocator(CommandListTypeName, COM_ARGS(&pool))); }
  CommandListResultType allocateList(ID3D12Device *device)
  {
    CommandListResultType result = {};
    if (!pool)
    {
      return result;
    }
    if (listsInUse < lists.size())
    {
      result = lists[listsInUse++];
      result->Reset(pool.Get(), nullptr);
    }
    else
    {
      CommandListStoreType newList;
      if (newList.autoQuery([=](auto uuid, auto ptr) //
            { return DX12_DEBUG_OK(device->CreateCommandList(0, CommandListTypeName, this->pool.Get(), nullptr, uuid, ptr)); }))
      {
        lists.push_back(eastl::move(newList));
        result = lists[listsInUse++];
      }
      else
      {
        logerr("DX12: Unable to allocate new command list");
        // can only happen when all CreateCommandList failed and this can only happen when the device was reset.
      }
    }
    return result;
  }
  void frameReset()
  {
    DX12_CHECK_RESULT(pool->Reset());
    listsInUse = 0;
  }
  void shutdown()
  {
    listsInUse = 0;
    lists.clear();
    pool.Reset();
  }
};
struct FrameInfo
{
  CommandStreamSet<AnyCommandListPtr, D3D12_COMMAND_LIST_TYPE_DIRECT> genericCommands;
  CommandStreamSet<AnyCommandListPtr, D3D12_COMMAND_LIST_TYPE_COMPUTE> computeCommands;
  CommandStreamSet<VersionedPtr<D3DCopyCommandList>, D3D12_COMMAND_LIST_TYPE_COPY> earlyUploadCommands;
  CommandStreamSet<VersionedPtr<D3DCopyCommandList>, D3D12_COMMAND_LIST_TYPE_COPY> lateUploadCommands;
  CommandStreamSet<VersionedPtr<D3DCopyCommandList>, D3D12_COMMAND_LIST_TYPE_COPY> readBackCommands;
  uint64_t progress = 0;
  EventPointer progressEvent{};
  eastl::vector<ProgramID> deletedPrograms;
  eastl::vector<GraphicsProgramID> deletedGraphicPrograms;
  eastl::vector<backend::ShaderModuleManager::AnyShaderModuleUniquePointer> deletedShaderModules;
  eastl::vector<Query *> deletedQueries;
  ShaderResourceViewDescriptorHeapManager resourceViewHeaps;
  SamplerDescriptorHeapManager samplerHeaps;
  BackendQueryManager backendQueryManager;

  void deleteProgram(ComputePipeline *compute_pipe);
  void deleteProgram(BasePipeline *graphics_pipe);
  void init(ID3D12Device *device);
  void shutdown(DeviceQueueGroup &queue_group, PipelineManager &pipe_man);
  // returns ticks waiting for gpu
  int64_t beginFrame(Device &device, DeviceQueueGroup &queue_group, PipelineManager &pipe_man);
  void preRecovery(DeviceQueueGroup &queue_group, PipelineManager &pipe_man);
  void recover(ID3D12Device *device);
};
constexpr uint32_t default_event_wait_time = 1;
// runs until check returns true, after some busy loop tries it starts to check the event state
template <typename T>
inline void try_and_wait_with_os_event(os_event_t &event, uint32_t spin_count, T check, uint32_t idle_time = default_event_wait_time)
{
  for (uint32_t i = 0; i < spin_count; ++i)
  {
    if (check())
      return;
    _mm_pause();
  }
  for (;;)
  {
    if (check())
      return;
    os_event_wait(&event, idle_time);
  }
}
// Argument handler will transform this into RangeType
template <typename T>
struct ExtraDataArray
{
  size_t index = 0;

  ExtraDataArray() = default;
  ExtraDataArray(size_t i) : index{i} {}

  using RangeType = eastl::span<T>;
};

using StringIndexRef = ExtraDataArray<const char>;
using WStringIndexRef = ExtraDataArray<const wchar_t>;

#if D3D_HAS_RAY_TRACING
using RaytraceGeometryDescriptionBufferResourceReferenceSetListRef =
  ExtraDataArray<const RaytraceGeometryDescriptionBufferResourceReferenceSet>;
using D3D12_RAYTRACING_GEOMETRY_DESC_ListRef = ExtraDataArray<const D3D12_RAYTRACING_GEOMETRY_DESC>;
#endif
using ImageViewStateListRef = ExtraDataArray<const ImageViewState>;
using ImagePointerListRef = ExtraDataArray<Image *const>;
using QueryPointerListRef = ExtraDataArray<Query *const>;
using D3D12_CPU_DESCRIPTOR_HANDLE_ListRef = ExtraDataArray<const D3D12_CPU_DESCRIPTOR_HANDLE>;
using BufferImageCopyListRef = ExtraDataArray<const BufferImageCopy>;
using BufferResourceReferenceAndShaderResourceViewListRef = ExtraDataArray<const BufferResourceReferenceAndShaderResourceView>;
using ViewportListRef = ExtraDataArray<const ViewportState>;
using ScissorRectListRef = ExtraDataArray<const D3D12_RECT>;

#define DX12_BEGIN_CONTEXT_COMMAND(name)            \
  struct Cmd##name : debug::call_stack::CommandData \
  {

#define DX12_BEGIN_CONTEXT_COMMAND_EXT_1(name, param0Type, param0Name) \
  struct Cmd##name : debug::call_stack::CommandData                    \
  {

#define DX12_BEGIN_CONTEXT_COMMAND_EXT_2(name, param0Type, param0Name, param1Type, param1Name) \
  struct Cmd##name : debug::call_stack::CommandData                                            \
  {

#define DX12_END_CONTEXT_COMMAND \
  }                              \
  ;

#define DX12_CONTEXT_COMMAND_PARAM(type, name)             type name;
#define DX12_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size) type name[size];
#include "device_context_cmd.h"
#undef DX12_BEGIN_CONTEXT_COMMAND
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_1
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_2
#undef DX12_END_CONTEXT_COMMAND
#undef DX12_CONTEXT_COMMAND_PARAM
#undef DX12_CONTEXT_COMMAND_PARAM_ARRAY

// need to shorten this - some Cmd structs can be combined, but this needs some refactoring in other places
class DeviceContext : protected ResourceUsageHistoryDataSetDebugger, public debug::call_stack::Generator //-V553
{
  template <typename T, typename... Args>
  T make_command(Args &&...args)
  {
    T cmd = T{this->generateCommandData(), eastl::forward<Args>(args)...};
    return cmd;
  }
  // Warning: intentionally not spinlock, since it has extremely bad behaviour
  // in case of high contention (e.g. during several threads of loading etc...)
  WinCritSec mutex;
  struct WorkerThread : public DaThread
  {
    WorkerThread(DeviceContext &c) : DaThread("DX12 Worker", 256 << 10, cpujobs::DEFAULT_THREAD_PRIORITY + 1), ctx(c) {}
    // calls device.processCommandPipe() until termination is requested
    void execute() override;
    DeviceContext &ctx;
    bool terminateIncoming = false;
  };
#if !FIXED_EXECUTION_MODE
  enum class ExecutionMode{
    CONCURRENT = 0,
    IMMEDIATE,
    IMMEDIATE_FLUSH,
  };
#endif
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
  struct BufferStateInfo
  {
    D3D12_RESOURCE_STATES state;
    uint32_t id;
  };

  using AnyCommandPack = TypePack<
#define DX12_BEGIN_CONTEXT_COMMAND(name)                               Cmd##name,
#define DX12_BEGIN_CONTEXT_COMMAND_EXT_1(name, param0Type, param0Name) ExtendedVariant<Cmd##name, param0Type>,
#define DX12_BEGIN_CONTEXT_COMMAND_EXT_2(name, param0Type, param0Name, param1Type, param1Name) \
  ExtendedVariant2<Cmd##name, param0Type, param1Type>,
#define DX12_END_CONTEXT_COMMAND
#define DX12_CONTEXT_COMMAND_PARAM(type, name)
#define DX12_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size)
#include "device_context_cmd.h"
#undef DX12_BEGIN_CONTEXT_COMMAND
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_1
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_2
#undef DX12_END_CONTEXT_COMMAND
#undef DX12_CONTEXT_COMMAND_PARAM
#undef DX12_CONTEXT_COMMAND_PARAM_ARRAY
    void>;

  using AnyCommandStore = VariantRingBuffer<AnyCommandPack>;

  enum
  {
    MAX_PENDING_WORK_ITEMS = 1,
  };

#if D3D_HAS_RAY_TRACING
  struct RaytraceState
  {
    RaytracePipeline *pipeline = nullptr;

    void onFlush() {}

    void onFrameStateInvalidate() { pipeline = nullptr; }
  };
#endif

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
    eastl::vector<SignatureInfo> signatures;
    eastl::vector<SignatureInfoEx> signaturesEx;

    ID3D12CommandSignature *getSignatureForStride(ID3D12Device *device, uint32_t stride, D3D12_INDIRECT_ARGUMENT_TYPE type)
    {
      auto ref = eastl::find_if(begin(signatures), end(signatures),
        [=](const SignatureInfo &si) { return si.type == type && si.stride == stride; });
      if (end(signatures) == ref)
      {
        D3D12_INDIRECT_ARGUMENT_DESC arg;
        D3D12_COMMAND_SIGNATURE_DESC desc;
        desc.NumArgumentDescs = 1;
        desc.pArgumentDescs = &arg;
        desc.NodeMask = 0;

        arg.Type = type;
        desc.ByteStride = stride;

        SignatureInfo info;
        info.stride = stride;
        info.type = type;
        if (!DX12_CHECK_OK(device->CreateCommandSignature(&desc, nullptr, COM_ARGS(&info.signature))))
        {
          return nullptr;
        }
        ref = signatures.insert(ref, eastl::move(info));
      }
      return ref->signature.Get();
    }
    ID3D12CommandSignature *getSignatureForStride(ID3D12Device *device, uint32_t stride, D3D12_INDIRECT_ARGUMENT_TYPE type,
      GraphicsPipelineSignature &signature)
    {
      uint8_t mask = signature.def.vertexShaderRegisters.specialConstantsMask |
                     signature.def.pixelShaderRegisters.specialConstantsMask |
                     signature.def.geometryShaderRegisters.specialConstantsMask |
                     signature.def.hullShaderRegisters.specialConstantsMask | signature.def.domainShaderRegisters.specialConstantsMask;
      bool has_draw_id = mask & dxil::SpecialConstantType::SC_DRAW_ID;
      if (!has_draw_id)
        return getSignatureForStride(device, stride, type);

      ID3D12RootSignature *root_signature = signature.signature.Get();
      auto ref = eastl::find_if(begin(signaturesEx), end(signaturesEx),
        [=](const SignatureInfoEx &si) { return si.type == type && si.stride == stride && si.rootSignature == root_signature; });
      if (end(signaturesEx) == ref)
      {
        D3D12_INDIRECT_ARGUMENT_DESC args[2] = {};
        D3D12_COMMAND_SIGNATURE_DESC desc;
        desc.NumArgumentDescs = 2;
        desc.pArgumentDescs = args;
        desc.NodeMask = 0;

        args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
        args[0].Constant.Num32BitValuesToSet = 1;
        args[0].Constant.RootParameterIndex = 0;
        args[0].Constant.DestOffsetIn32BitValues = 0;
        args[1].Type = type;
        desc.ByteStride = stride;

        SignatureInfoEx info;
        info.stride = stride;
        info.type = type;
        info.rootSignature = root_signature;
        if (!DX12_CHECK_OK(device->CreateCommandSignature(&desc, root_signature, COM_ARGS(&info.signature))))
        {
          return nullptr;
        }
        ref = signaturesEx.insert(ref, eastl::move(info));
      }
      return ref->signature.Get();
    }
    void reset()
    {
      signatures.clear();
      signaturesEx.clear();
    }
  };

  // ContextState
  struct ContextState : public debug::DeviceContextState
  {
    ForwardRing<FrameInfo, FRAME_FRAME_BACKLOG_LENGTH> frames;
    SignatureStore drawIndirectSignatures;
    SignatureStore drawIndexedIndirectSignatures;
    ComPtr<ID3D12CommandSignature> dispatchIndirectSignature;
    BarrierBatcher uploadBarrierBatch;
    BarrierBatcher postUploadBarrierBatch;
    BarrierBatcher graphicsCommandListBarrierBatch;
    SplitTransitionTracker graphicsCommandListSplitBarrierTracker;
    InititalResourceStateSet initialResourceStateSet;
    ResourceUsageManagerWithHistory resourceStates;
    ResourceActivationTracker resourceActivationTracker;
    BufferAccessTracker bufferAccessTracker;
    ID3D12Resource *lastAliasBegin = nullptr;
    FramebufferLayoutManager framebufferLayouts;
    eastl::vector<eastl::pair<Image *, SubresourceIndex>> textureReadBackSplitBarrierEnds;
    CopyCommandList<VersionedPtr<D3DCopyCommandList>> activeReadBackCommandList;
    CopyCommandList<VersionedPtr<D3DCopyCommandList>> activeEarlyUploadCommandList;
    CopyCommandList<VersionedPtr<D3DCopyCommandList>> activeLateUploadCommandList;

    backend::BindlessSetManager bindlessSetManager;

    GraphicsState graphicsState;
    ComputeState computeState;
#if D3D_HAS_RAY_TRACING
    RaytraceState raytraceState;
#endif
    PipelineStageStateBase stageState[STAGE_MAX_EXT];

    StatefulCommandBuffer cmdBuffer = {};
    eastl::vector<eastl::pair<size_t, size_t>> renderTargetSplitStarts;

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
    }

    void purgeAllBindings()
    {
      for (auto &stage : stageState)
      {
        stage.resetAllState();
      }
    }

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

    FrameInfo &getFrameData() { return frames.get(); }
  };

  class ExecutionContext : public stackhelp::ext::ScopedCallStackContext
  {
    friend class DeviceContext;
    Device &device;
    // link to owner, needed to access device and other objects
    DeviceContext &self;
    size_t cmd = 0;
    uint32_t cmdIndex = 0;

    ContextState &contextState;

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
    bool readyEarlyUploadCommandList();
    bool readyLateUploadCommandList();

#if _TARGET_PC_WIN
    bool readyCommandList() const { return contextState.cmdBuffer.isReadyForRecording(); }
#else
    constexpr bool readyCommandList() const { return true; }
#endif

    AnyCommandListPtr allocAndBeginCommandBuffer();
    void readBackImagePrePhase();
    bool checkDrawCallHasOutput(eastl::span<const char> info);
    template <size_t N>
    bool checkDrawCallHasOutput(const char (&sl)[N])
    {
      return checkDrawCallHasOutput(string_literal_span(sl));
    }

  public:
    ExecutionContext(DeviceContext &ctx, ContextState &css);
    void beginWork();
    void beginCmd(size_t icmd);
    void endCmd();
    FramebufferState &getFramebufferState();
    void setUniformBuffer(uint32_t stage, uint32_t unit, BufferResourceReferenceAndAddressRange buffer);
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
    bool isPartOfFramebuffer(Image *image);
    bool isPartOfFramebuffer(Image *image, MipMapRange mip_range, ArrayLayerRange array_range);
    // returns true if the current pass encapsulates the draw area of the viewport
    void updateViewports(ViewportListRef::RangeType new_vps);
    D3D12_CPU_DESCRIPTOR_HANDLE getNullColorTarget();
    void setStaticRenderState(StaticRenderStateID ident);
    void setInputLayout(InputLayoutID ident);
    void setWireFrame(bool wf);
    void prepareCommandExecution();
    void setConstRegisterBuffer(uint32_t stage, HostDeviceSharedMemoryRegion update);
    void writeToDebug(StringIndexRef::RangeType index);
    // mode only used if present is true
    int64_t flush(bool present, uint64_t progress, OutputMode mode);
    void pushEvent(StringIndexRef::RangeType name);
    void popEvent();
    void writeTimestamp(Query *query);
    void wait(uint64_t value);
    void beginSurvey(PredicateInfo pi);
    void endSurvey(PredicateInfo pi);
    void beginConditionalRender(PredicateInfo pi);
    void endConditionalRender();
    void addVertexShader(ShaderID id, VertexShaderModule *sci);
    void addPixelShader(ShaderID id, PixelShaderModule *sci);
    void addComputePipeline(ProgramID id, ComputeShaderModule *csm);
    void addGraphicsPipeline(GraphicsProgramID program, ShaderID vs, ShaderID ps);
#if D3D_HAS_RAY_TRACING
    void addRaytracePipeline(ProgramID program, uint32_t max_recursion, uint32_t shader_count, const ShaderID *shaders,
      uint32_t group_count, const RaytraceShaderGroup *groups);
    void copyRaytraceShaderGroupHandlesToMemory(ProgramID program, uint32_t first_group, uint32_t group_count, uint32_t size,
      void *ptr);
    void buildBottomAccelerationStructure(D3D12_RAYTRACING_GEOMETRY_DESC_ListRef::RangeType geometry_descriptions,
      RaytraceGeometryDescriptionBufferResourceReferenceSetListRef::RangeType resource_refs,
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, bool update, ID3D12Resource *dst, ID3D12Resource *src,
      BufferResourceReferenceAndAddress scratch);
    void buildTopAccelerationStructure(uint32_t instance_count, BufferResourceReferenceAndAddress instance_buffer,
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, bool update, ID3D12Resource *dst, ID3D12Resource *src,
      BufferResourceReferenceAndAddress scratch);
    void traceRays(BufferResourceReferenceAndRange ray_gen_table, BufferResourceReferenceAndRange miss_table,
      BufferResourceReferenceAndRange hit_table, BufferResourceReferenceAndRange callable_table, uint32_t miss_stride,
      uint32_t hit_stride, uint32_t callable_stride, uint32_t width, uint32_t height, uint32_t depth);
#endif
    void present(uint64_t progress, Drv3dTimings *timing_data, int64_t kickoff_stamp, uint32_t latency_frame, OutputMode mode);
    void dispatch(uint32_t x, uint32_t y, uint32_t z);
    void dispatchIndirect(BufferResourceReferenceAndOffset buffer);
    void copyBuffer(BufferResourceReferenceAndOffset src, BufferResourceReferenceAndOffset dst, uint32_t size);
    void updateBuffer(HostDeviceSharedMemoryRegion update, BufferResourceReferenceAndOffset dest);
    void clearBufferFloat(BufferResourceReferenceAndClearView buffer, const float values[4]);
    void clearBufferUint(BufferResourceReferenceAndClearView buffer, const uint32_t values[4]);
    void clearDepthStencilImage(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor,
      const ClearDepthStencilValue &value);
    void clearColorImage(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor, const ClearColorValue &value);
    void copyImage(Image *src, Image *dst, const ImageCopy &copy);
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
#if D3D_HAS_RAY_TRACING
    void setRaytracePipeline(ProgramID program);
#endif
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
    void flushGraphicsStateRessourceBindings();
    // handled by flush, could be moved into this though
    void ensureActivePass();
    void changePresentMode(PresentationMode mode);
    void updateVertexShaderName(ShaderID shader, StringIndexRef::RangeType name);
    void updatePixelShaderName(ShaderID shader, StringIndexRef::RangeType name);
    void clearUAVTextureI(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor, const uint32_t values[4]);
    void clearUAVTextureF(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor, const float values[4]);
    void setGamma(float power);
    void setComputeRootConstant(uint32_t offset, uint32_t value);
    void setVertexRootConstant(uint32_t offset, uint32_t value);
    void setPixelRootConstant(uint32_t offset, uint32_t value);
#if D3D_HAS_RAY_TRACING
    void setRaytraceRootConstant(uint32_t offset, uint32_t value);
#endif
    void registerStaticRenderState(StaticRenderStateID ident, const RenderStateSystem::StaticState &state);
    void beginVisibilityQuery(Query *q);
    void endVisibilityQuery(Query *q);
    void flushComputeState();
    void textureReadBack(Image *image, HostDeviceSharedMemoryRegion cpu_memory, BufferImageCopyListRef::RangeType regions,
      DeviceQueueType queue);
    void bufferReadBack(BufferResourceReferenceAndRange buffer, HostDeviceSharedMemoryRegion cpu_memory, size_t offset);
#if !_TARGET_XBOXONE
    void setVariableRateShading(D3D12_SHADING_RATE rate, D3D12_SHADING_RATE_COMBINER vs_combiner,
      D3D12_SHADING_RATE_COMBINER ps_combiner);
    void setVariableRateShadingTexture(Image *texture);
#endif
    void registerInputLayout(InputLayoutID ident, const InputLayout &layout);
    void createDlssFeature(int dlss_quality, Extent2D output_resolution, bool stereo_render);
    void releaseDlssFeature();
    void prepareExecuteAA(Image *inColor, Image *inDepth, Image *inMotionVectors, Image *outColor);
    void executeDlss(const DlssParamsDx12 &dlss_params, int view_index);
    void executeXess(const XessParamsDx12 &params);
    void executeFSR2(const Fsr2ParamsDx12 &params);
    void removeVertexShader(ShaderID shader);
    void removePixelShader(ShaderID shader);
    void deleteProgram(ProgramID program);
    void deleteGraphicsProgram(GraphicsProgramID program);
    void deleteQueries(QueryPointerListRef::RangeType queries);
    void hostToDeviceMemoryCopy(BufferResourceReferenceAndRange target, HostDeviceSharedMemoryRegion source, size_t source_offset);
    void initializeTextureState(D3D12_RESOURCE_STATES state, ValueRange<ExtendedImageGlobalSubresouceId> id_range);
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

#if _TARGET_PC_WIN
    void onDeviceError(HRESULT remove_reason);
    void onSwapchainSwapCompleted();
#endif

#if _TARGET_PC_WIN
    void beginTileMapping(Image *image, ID3D12Heap *heap, size_t heap_base, size_t mapping_count);
#else
    void beginTileMapping(Image *image, uintptr_t address, uint32_t size, size_t mapping_count);
#endif
    void addTileMappings(const TileMapping *mapping, size_t mapping_count);
    void endTileMapping();

    void activateBuffer(BufferResourceReferenceAndAddressRangeWithClearView buffer, const ResourceMemory &memory,
      ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline gpu_pipeline);
    void activateTexture(Image *tex, ResourceActivationAction action, const ResourceClearValue &value, ImageViewState view_state,
      D3D12_CPU_DESCRIPTOR_HANDLE view, GpuPipeline gpu_pipeline);
    void deactivateBuffer(BufferResourceReferenceAndAddressRange buffer, const ResourceMemory &memory, GpuPipeline gpu_pipeline);
    void deactivateTexture(Image *tex, GpuPipeline gpu_pipeline);
    void aliasFlush(GpuPipeline gpu_pipeline);
    void twoPhaseCopyBuffer(BufferResourceReferenceAndOffset source, uint32_t destination_offset, ScratchBuffer scratch_memory,
      uint32_t data_size);

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
    void addShaderGroup(uint32_t group, ScriptedShadersBinDumpOwner *dump, ShaderID null_pixel_shader);
    void removeShaderGroup(uint32_t group);
    void loadComputeShaderFromDump(ProgramID program);
    void compilePipelineSet(DynamicArray<InputLayoutID> &&input_layouts, DynamicArray<StaticRenderStateID> &&static_render_states,
      DynamicArray<FramebufferLayout> &&framebuffer_layouts, DynamicArray<GraphicsPipelinePreloadInfo> &&graphics_pipelines,
      DynamicArray<MeshPipelinePreloadInfo> &&mesh_pipelines, DynamicArray<ComputePipelinePreloadInfo> &&compute_pipelines);
  };

#if DAGOR_DBGLEVEL > 0
  static constexpr size_t NumFrameCommandLogs = 5;

  struct FrameCommandLog
  {
    VariantVectorRingBuffer<AnyCommandPack> log;
    uint32_t frameId = 0;
  };

  eastl::array<FrameCommandLog, NumFrameCommandLogs> frameLogs;
  uint32_t activeLogId = 0;
  uint64_t frameId = 0;

  VariantVectorRingBuffer<AnyCommandPack> &GetActiveFrameLog() { return frameLogs[activeLogId].log; }

  void initNextFrameLog();
  void dumpCommandLog();

  template <typename T, typename P0, typename P1>
  void logCommand(const ExtendedVariant2<T, P0, P1> &params)
  {
    GetActiveFrameLog().pushBack<T, P0, P1>(params.cmd, params.p0.size(), [&params](auto index, auto first, auto second) {
      first(params.p0[index]);
      second(params.p1[index]);
    });
  }

  template <typename T, typename P0>
  void logCommand(const ExtendedVariant<T, P0> &params)
  {
    GetActiveFrameLog().pushBack<T, P0>(params.cmd, params.p0.data(), params.p0.size());
  }

  template <typename T>
  void logCommand(T cmd)
  {
    GetActiveFrameLog().pushBack(cmd);
  }
#else
  template <typename T>
  void logCommand(T)
  {}
#endif

  struct FrontendFrameLatchedData
  {
    uint64_t progress = 0;
    uint32_t frameIndex = 0;
    eastl::vector<Query *> deletedQueries;
  };

  struct Frontend
  {
    FrontendFrameLatchedData latchedFrameSet[FRAME_FRAME_BACKLOG_LENGTH];
    EventPointer frameWaitEvent;
    FrontendFrameLatchedData *recordingLatchedFrame = nullptr;
    uint32_t activeRangedQueries = 0;
    uint32_t frameIndex = 0;
    uint64_t nextWorkItemProgress = 2;
    uint64_t recordingWorkItemProgress = 1;
    uint64_t completedFrameProgress = 0;
    eastl::vector<size_t> renderTargetIndices;
    // atomic so multiple threads are able to safely check if the progress has been completed
    std::atomic<uint64_t> frontendFinishedWorkItem = {0};
    eastl::vector<FrameEvents *> frameEventCallbacks;
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
    BindlessSetId lastBindlessSetId = BindlessSetId::Null();
    eastl::vector<EventPointer> eventsPool;
    frontend::Swapchain swapchain;
  };
  struct Backend
  {
    // state used by any context
    ContextState sharedContextState;
    eastl::vector<os_event_t> frameCompleteEvents;
    eastl::vector<FrameEvents *> frameEventCallbacks;
#if DX12_RECORD_TIMING_DATA
    int64_t gpuWaitDuration = 0;
    int64_t acquireBackBufferDuration = 0;
    int64_t workWaitDuration = 0;
#endif
    ConstBufferStreamDescriptorHeap constBufferStreamDescriptors;
    backend::Swapchain swapchain{};
    // The backend has its own frame progress counter, as its simpler than syncing
    // it with the front end one.
    uint64_t frameProgress = 0;
  };
  friend class Device;
  friend class frontend::Swapchain;
  friend class backend::Swapchain;
  Device &device;

  AnyCommandStore commandStream;
  eastl::unique_ptr<WorkerThread> worker;
#if !FIXED_EXECUTION_MODE
  ExecutionMode executionMode = ExecutionMode::IMMEDIATE;
  bool isImmediateFlushSuppressed = false;
#endif
  Frontend front;
  Backend back = {};
  // shared::Swapchain swapchain;
  // WinCritSec stubSwapGuard;
#if _TARGET_XBOX
  EventPointer enteredSuspendedStateEvent;
  EventPointer resumeExecutionEvent;
#endif
#if DX12_REPORT_DISCARD_MEMORY_PER_FRAME
  std::atomic<size_t> discardBytes{0};
#endif

  XessWrapper xessWrapper;
  NgxWrapper ngxWrapper;
  Fsr2Wrapper fsr2Wrapper;

#if FIXED_EXECUTION_MODE
  constexpr bool isImmediateMode() const { return false; }
  constexpr bool isImmediateFlushMode() const { return false; }
  constexpr void immediateModeExecute(bool = false) const {}
  constexpr void suppressImmediateFlush() {}
  constexpr void restoreImmediateFlush() {}
#else
  bool isImmediateMode() const { return ExecutionMode::CONCURRENT != executionMode; }
  bool isImmediateFlushMode() const { return ExecutionMode::IMMEDIATE_FLUSH == executionMode; }
  void suppressImmediateFlush()
  {
    if (executionMode == ExecutionMode::IMMEDIATE_FLUSH)
    {
      executionMode = ExecutionMode::IMMEDIATE;
      isImmediateFlushSuppressed = true;
    }
  }
  void restoreImmediateFlush()
  {
    if (isImmediateFlushSuppressed)
    {
      isImmediateFlushSuppressed = false;
      executionMode = ExecutionMode::IMMEDIATE_FLUSH;
    }
  }

  // if immediate mode is enabled then this executes all queued commands in front.commandStream and
  // if immediate flush mode is enabled and the flush parameter is true then current thread will wait
  // for the GPU to finish the commands clears the stream after the execution
  void immediateModeExecute(bool flush = false);
#endif

  void replayCommands();
  void replayCommandsConcurrently(volatile int &terminate);

  void frontFlush();
  void manageLatchedState();

  void makeReadyForFrame(OutputMode mode);
  void initFrameStates();
  void shutdownFrameStates();
  WinCritSec &getFrontGuard();
#if FIXED_EXECUTION_MODE
  void initMode();
#else
  void initMode(ExecutionMode mode);
#endif
  void shutdownWorkerThread()
  {
#if !FIXED_EXECUTION_MODE
    if (ExecutionMode::CONCURRENT == executionMode)
#endif
    {
      if (worker)
      {
        commandStream.pushBack(make_command<CmdTerminate>());
        // tell the worker to terminate
        // it will execute all pending commands
        // and then clean up and shutdown
        worker->terminate(true);
        worker.reset();
      }
      // switch to immediate mode if something else should come up
#if !FIXED_EXECUTION_MODE
      executionMode = ExecutionMode::IMMEDIATE;
#endif
    }
  }

  // assumes frontend and backend are synced and locked so that access to front and back is safe
  void resizeSwapchain(Extent2D size);
  void waitInternal();
  void finishInternal();
  void blitImageInternal(Image *src, Image *dst, const ImageBlit &region, bool disable_predication);
  void tidyFrame(FrontendFrameLatchedData &frame);
#if _TARGET_PC_WIN
  void onDeviceError(HRESULT remove_reason);
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
  void setConstBuffer(uint32_t stage, size_t unit, BufferResourceReferenceAndAddressRange buffer);

  void setSRVNull(uint32_t stage, uint32_t unit);
  void setUAVNull(uint32_t stage, uint32_t unit);

  void setBlendConstant(E3DCOLOR color);
  void setDepthBoundsRange(float from, float to);
  void setPolygonLine(bool enable);
  void setStencilRef(uint8_t ref);
  void setScissorEnable(bool enabled);
  void setScissorRects(dag::ConstSpan<D3D12_RECT> rects);

  void bindVertexDecl(InputLayoutID ident);

  void setIndexBuffer(BufferResourceReferenceAndAddressRange buffer, DXGI_FORMAT type);

  void bindVertexBuffer(uint32_t stream, BufferResourceReferenceAndAddressRange buffer, uint32_t stride);

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
  void clearDepthStencilImage(Image *image, const ImageSubresourceRange &area, const ClearDepthStencilValue &value);
  void clearColorImage(Image *image, const ImageSubresourceRange &area, const ClearColorValue &value);
  void copyImage(Image *src, Image *dst, const ImageCopy &copy);
  void flushDraws();
  // Similar to flushDraws, with the exception that it will only execute a flush when no queries are active.
  // Returns true if it executed a flush, otherwise it returns false.
  bool flushDrawWhenNoQueries();
  void blitImage(Image *src, Image *dst, const ImageBlit &region);
  void wait();
  void beginSurvey(int name);
  void endSurvey(int name);
  void destroyBuffer(BufferState buffer, const char *name);
  BufferState discardBuffer(BufferState to_discared, DeviceMemoryClass memory_class, FormatStore format, uint32_t struct_size,
    bool raw_view, bool struct_view, D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name);
  void destroyImage(Image *img);
  void present(OutputMode mode);
  void changePresentMode(PresentationMode mode);
  void changeSwapchainExtents(Extent2D size);
#if _TARGET_PC_WIN
  void changeFullscreenExclusiveMode(bool is_exclusive);
  void changeFullscreenExclusiveMode(bool is_exclusive, ComPtr<IDXGIOutput> output);
  HRESULT getSwapchainDesc(DXGI_SWAP_CHAIN_DESC *out_desc);
  IDXGIOutput *getSwapchainOutput();
#endif
  void shutdownSwapchain();
  void insertTimestampQuery(Query *query);
  void deleteQuery(Query *query);
  void generateMipmaps(Image *img);
  void setFramebuffer(Image **image_list, ImageViewState *view_list, bool read_only_depth);
#if D3D_HAS_RAY_TRACING
  void raytraceBuildBottomAccelerationStructure(RaytraceBottomAccelerationStructure *as, RaytraceGeometryDescription *desc,
    uint32_t count, RaytraceBuildFlags flags, bool update, BufferResourceReferenceAndAddress scratch_buf);
  void raytraceBuildTopAccelerationStructure(RaytraceTopAccelerationStructure *as, BufferReference index_buffer, uint32_t index_count,
    RaytraceBuildFlags flags, bool update, BufferResourceReferenceAndAddress scratch_buf);
  void deleteRaytraceBottomAccelerationStructure(RaytraceBottomAccelerationStructure *desc);
  void deleteRaytraceTopAccelerationStructure(RaytraceTopAccelerationStructure *desc);
  void traceRays(BufferResourceReferenceAndRange ray_gen_table, BufferResourceReferenceAndRange miss_table, uint32_t miss_stride,
    BufferResourceReferenceAndRange hit_table, uint32_t hit_stride, BufferResourceReferenceAndRange callable_table,
    uint32_t callable_stride, uint32_t width, uint32_t height, uint32_t depth);
  void setRaytracePipeline(ProgramID program);
  void setRaytraceAccelerationStructure(uint32_t stage, size_t unit, RaytraceAccelerationStructure *as);
#endif
  void beginConditionalRender(int name);
  void endConditionalRender();

  void addVertexShader(ShaderID id, eastl::unique_ptr<VertexShaderModule> shader);
  void addPixelShader(ShaderID id, eastl::unique_ptr<PixelShaderModule> shader);
  void removeVertexShader(ShaderID id);
  void removePixelShader(ShaderID id);

  void addGraphicsProgram(GraphicsProgramID program, ShaderID vs, ShaderID ps);
  void addComputeProgram(ProgramID id, eastl::unique_ptr<ComputeShaderModule> csm);
  void removeProgram(ProgramID program);

#if D3D_HAS_RAY_TRACING
  void addRaytraceProgram(ProgramID program, uint32_t max_recursion, uint32_t shader_count, const ShaderID *shaders,
    uint32_t group_count, const RaytraceShaderGroup *groups);
  void copyRaytraceShaderGroupHandlesToMemory(ProgramID prog, uint32_t first_group, uint32_t group_count, uint32_t size,
    BufferResourceReference buffer, uint32_t offset);
#endif

  void placeAftermathMarker(const char *name);
  void updateVertexShaderName(ShaderID shader, const char *name);
  void updatePixelShaderName(ShaderID shader, const char *name);
  void setImageResourceState(D3D12_RESOURCE_STATES state, ValueRange<ExtendedImageGlobalSubresouceId> range);
  void setImageResourceStateNoLock(D3D12_RESOURCE_STATES state, ValueRange<ExtendedImageGlobalSubresouceId> range);
  void clearUAVTexture(Image *image, ImageViewState view, const unsigned values[4]);
  void clearUAVTexture(Image *image, ImageViewState view, const float values[4]);
  void setGamma(float power);
  /*WinCritSec &getStubSwapGuard()
  {
    return stubSwapGuard;
  }*/
  void setComputeRootConstant(uint32_t offset, uint32_t size);
  void setVertexRootConstant(uint32_t offset, uint32_t size);
  void setPixelRootConstant(uint32_t offset, uint32_t size);
#if D3D_HAS_RAY_TRACING
  void setRaytraceRootConstant(uint32_t offset, uint32_t size);
#endif
#if _TARGET_PC_WIN
  void preRecovery();
  void recover(const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> &unbounded_samplers);
#endif
  void deleteTexture(BaseTex *tex);
  void freeMemory(HostDeviceSharedMemoryRegion allocation);
  void freeMemoryOfUploadBuffer(HostDeviceSharedMemoryRegion allocation);
  void uploadToBuffer(BufferResourceReferenceAndRange target, HostDeviceSharedMemoryRegion memory, size_t m_offset);
  void readBackFromBuffer(HostDeviceSharedMemoryRegion memory, size_t m_offset, BufferResourceReferenceAndRange source);
  void uploadToImage(Image *target, const BufferImageCopy *regions, uint32_t region_count, HostDeviceSharedMemoryRegion memory,
    DeviceQueueType queue, bool is_discard);
  // Return value is the progress the caller can wait on to ensure completion of this operation
  uint64_t readBackFromImage(HostDeviceSharedMemoryRegion memory, const BufferImageCopy *regions, uint32_t region_count, Image *source,
    DeviceQueueType queue);
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
  void resummarizeHtile(ID3D12Resource *depth);
#endif
  // Returns the progress we are recording now for future push to the GPU
  uint64_t getRecordingFenceProgress() { return front.recordingWorkItemProgress; }
  // Returns current progress of the GPU
  uint64_t getCompletedFenceProgress();
  // Returns current progress of the frontend of completed work items, including cleanup
  uint64_t getFinishedFenceProgress()
  {
    // relaxed is ok here, we only require that the load it self is atomic, we don't need any ordering guarantees.
    return front.frontendFinishedWorkItem.load(std::memory_order_relaxed);
  }
  void waitForProgress(uint64_t progress);
  void beginStateCommit() { mutex.lock(); }
  void endStateCommit() { mutex.unlock(); }
#if !_TARGET_XBOXONE
  void setVariableRateShading(D3D12_SHADING_RATE rate, D3D12_SHADING_RATE_COMBINER vs_combiner,
    D3D12_SHADING_RATE_COMBINER ps_combiner);
  void setVariableRateShadingTexture(Image *texture);
#endif
  void registerInputLayout(InputLayoutID ident, const InputLayout &layout);
  void initNgx(bool stereo_render);
  void initXeSS();
  void initFsr2();
  void shutdownNgx();
  void shutdownXess();
  void shutdownFsr2();
  DlssState getDlssState() const { return ngxWrapper.getDlssState(); }
  XessState getXessState() const { return xessWrapper.getXessState(); }
  Fsr2State getFsr2State() const { return fsr2Wrapper.getFsr2State(); }
  uint64_t getDlssVramUsage()
  {
    unsigned long long vramUsage = 0;
    ngxWrapper.dlssGetStats(&vramUsage);
    return vramUsage;
  }
  bool isDlssQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int dlss_quality) const
  {
    return ngxWrapper.isDlssQualityAvailableAtResolution(target_width, target_height, dlss_quality);
  }
  bool isXessQualityAvailableAtResolution(uint32_t target_width, uint32_t target_height, int xess_quality) const
  {
    return xessWrapper.isXessQualityAvailableAtResolution(target_width, target_height, xess_quality);
  }
  void getDlssRenderResolution(int &w, int &h) const { ngxWrapper.getDlssRenderResolution(w, h); }
  void getXessRenderResolution(int &w, int &h) const { xessWrapper.getXeSSRenderResolution(w, h); }
  void getFsr2RenderResolution(int &width, int &height) { fsr2Wrapper.getFsr2RenderingResolution(width, height); }
  void setXessVelocityScale(float x, float y) { xessWrapper.setVelocityScale(x, y); }
  void createDlssFeature(int dlss_quality, Extent2D output_resolution, bool stereo_render);
  void releaseDlssFeature();
  void executeDlss(const DlssParams &dlss_params, int view_index);
  void executeXess(const XessParams &params);
  void executeFSR2(const Fsr2Params &params);
  void bufferBarrier(BufferResourceReference buffer, ResourceBarrier barrier, GpuPipeline queue);
  void textureBarrier(Image *tex, SubresourceRange sub_res_range, uint32_t tex_flags, ResourceBarrier barrier, GpuPipeline queue,
    bool force_barrier);
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
  void bindlessSetResourceDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
  void bindlessSetResourceDescriptor(uint32_t slot, Image *texture, ImageViewState view);
  void bindlessSetSamplerDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
  void bindlessCopyResourceDescriptors(uint32_t src, uint32_t dst, uint32_t count);

  BaseTex *getSwapchainColorTexture();
  BaseTex *getSwapchainSecondaryColorTexture();
  BaseTex *getSwapchainDepthStencilTexture(Extent2D ext);
  BaseTex *getSwapchainDepthStencilTextureAnySize();
  Extent2D getSwapchainExtent() const;
  bool isVrrSupported() const;
  bool isVsyncOn() const;
  FormatStore getSwapchainDepthStencilFormat() const;
  FormatStore getSwapchainColorFormat() const;
  FormatStore getSwapchainSecondaryColorFormat() const;
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

  bool hasWorkerThread() const
  {
#if !FIXED_EXECUTION_MODE
    return executionMode == ExecutionMode::CONCURRENT;
#else
    return true;
#endif
  }

  bool wasCurrentFramePresentSubmitted() const
  {
#if _TARGET_PC_WIN
    if (hasWorkerThread())
    {
      return back.swapchain.wasCurrentFramePresentSubmitted();
    }
#endif
    return false;
  }

  bool swapchainPresentFromMainThread()
  {
#if _TARGET_PC_WIN
    if (hasWorkerThread())
    {
      return back.swapchain.swapchainPresentFromMainThread(*this);
    }
#endif
    return true;
  }

#if _TARGET_PC_WIN
  void onSwapchainSwapCompleted();
#endif

  void mapTileToResource(BaseTex *tex, ResourceHeap *heap, const TileMapping *mapping, size_t mapping_count);

  void freeUserHeap(::ResourceHeap *ptr);
  void activateBuffer(BufferResourceReferenceAndAddressRangeWithClearView buffer, const ResourceMemory &memory,
    ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline gpu_pipeline);
  void activateTexture(BaseTex *texture, ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline gpu_pipeline);
  void deactivateBuffer(BufferResourceReferenceAndAddressRange buffer, const ResourceMemory &memory, GpuPipeline gpu_pipeline);
  void deactivateTexture(Image *tex, GpuPipeline gpu_pipeline);
  void aliasFlush(GpuPipeline gpu_pipeline);
  HostDeviceSharedMemoryRegion allocatePushMemory(uint32_t size, uint32_t alignment);


#if FIXED_EXECUTION_MODE
  constexpr bool enableImmediateFlush() { return false; }
  constexpr void disableImmediateFlush() {}
#else
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
  void addShaderGroup(uint32_t group, ScriptedShadersBinDumpOwner *dump, ShaderID null_pixel_shader);
  void removeShaderGroup(uint32_t group);
  void loadComputeShaderFromDump(ProgramID program);
  void compilePipelineSet(const DataBlock *feature_sets, DynamicArray<InputLayoutID> &&input_layouts,
    DynamicArray<StaticRenderStateID> &&static_render_states, const DataBlock *output_formats_set,
    const DataBlock *graphics_pipeline_set, const DataBlock *mesh_pipeline_set, const DataBlock *compute_pipeline_set,
    const char *default_format);
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
