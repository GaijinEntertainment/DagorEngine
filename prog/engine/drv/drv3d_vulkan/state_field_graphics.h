// fields that related to graphics part of pipeline
#pragma once
#include "util/tracked_state.h"
#include "driver.h"
#include "graphics_state.h"

namespace drv3d_vulkan
{

struct BackGraphicsStateStorage;
struct FrontGraphicsStateStorage;
struct BackDynamicGraphicsStateStorage;
struct BufferRef;
class RenderPassClass;
class VariatedGraphicsPipeline;
class GraphicsPipeline;
class GraphicsPipelineLayout;
class ExecutionContext;

// front fields

struct StateFieldGraphicsRenderState : TrackedStateFieldBase<true, false>,
                                       TrackedStateFieldGenericSmallPOD<shaders::DriverRenderStateId>
{
  template <typename StorageType>
  void reset(StorageType &)
  {
    data = (shaders::DriverRenderStateId)0;
  }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsPolygonLine : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericSmallPOD<bool>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsProgram : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericTaggedHandle<ProgramID>
{
  ProgramID &getValue() { return handle; }
  const ProgramID &getValueRO() const { return handle; }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsInputLayoutOverride : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericTaggedHandle<InputLayoutID>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct ConditionalRenderingState
{
  using This = ConditionalRenderingState;
  struct InvalidateTag
  {};

  VulkanBufferHandle buffer{};
  VkDeviceSize offset{0};

  friend bool operator==(const This &left, const This &right) { return left.buffer == right.buffer && left.offset == right.offset; }

  friend bool operator!=(const This &left, const This &right) { return !(left == right); }
};

// Tracks the location for conditional rendering desired by the front end.
// As per the spec, no more than 1 can be active at a time, so we
// do not need a list here
struct StateFieldGraphicsConditionalRenderingState : TrackedStateFieldBase<true, false>,
                                                     TrackedStateFieldGenericPOD<ConditionalRenderingState>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

// back fields

struct StateFieldGraphicsConditionalRenderingScopeOpener : TrackedStateFieldBase<true, false>,
                                                           TrackedStateFieldGenericPOD<ConditionalRenderingState>
{
  // Bring parent's function into this scope so that they get included
  // into the overload sets on name resolution (otherwise they would not
  // get considered at all...).
  using TrackedStateFieldGenericPOD<ConditionalRenderingState>::set;
  using TrackedStateFieldGenericPOD<ConditionalRenderingState>::diff;

  void set(ConditionalRenderingState::InvalidateTag) {}
  bool diff(ConditionalRenderingState::InvalidateTag) const { return true; }

  template <typename Storage, typename Context>
  void applyTo(Storage &data, Context &target);
  template <typename Storage>
  void dumpLog(Storage &data) const;
};

struct StateFieldGraphicsRenderPassScopeCloser : TrackedStateFieldBase<true, false>
{
  bool data;
  bool shouldClose;
  ActiveExecutionStage stage;

  template <typename StorageType>
  void reset(StorageType &)
  {
    data = {};
    stage = ActiveExecutionStage::FRAME_BEGIN;
    shouldClose = false;
  }

  void set(const ActiveExecutionStage to) { stage = to; }
  bool diff(const ActiveExecutionStage) const { return true; }

  void set(const bool &value) { data = value; }
  bool diff(const bool &) const { return true; }

  template <typename Storage, typename Context>
  void applyTo(Storage &data, Context &target);
  template <typename Storage>
  void dumpLog(Storage &data) const;
};

struct StateFieldGraphicsNativeRenderPassScopeCloser : TrackedStateFieldBase<true, false>
{
  bool data;
  bool shouldClose;
  ActiveExecutionStage stage;

  template <typename StorageType>
  void reset(StorageType &)
  {
    data = {};
    stage = ActiveExecutionStage::FRAME_BEGIN;
    shouldClose = false;
  }

  void set(const ActiveExecutionStage to) { stage = to; }
  bool diff(const ActiveExecutionStage) const { return true; }

  void set(const bool &value) { data = value; }
  bool diff(const bool &) const { return true; }

  template <typename Storage, typename Context>
  void applyTo(Storage &data, Context &target);
  template <typename Storage>
  void dumpLog(Storage &data) const;
};

struct StateFieldGraphicsConditionalRenderingScopeCloser : TrackedStateFieldBase<true, false>
{
  ConditionalRenderingState data;
  ActiveExecutionStage stage;
  bool shouldClose;

  template <typename StorageType>
  void reset(StorageType &)
  {
    data = {};
    stage = ActiveExecutionStage::FRAME_BEGIN;
    shouldClose = false;
  }

  // TODO: When more scope-based fields appear, consider generalizing
  // this code via templates.

  void set(const ActiveExecutionStage to) { stage = to; }
  bool diff(const ActiveExecutionStage) const { return true; }

  void set(const ConditionalRenderingState &value) { data = value; }
  bool diff(const ConditionalRenderingState &value) const { return data != value; }

  void set(ConditionalRenderingState::InvalidateTag) {}
  bool diff(ConditionalRenderingState::InvalidateTag) const { return shouldClose; }

  template <typename Storage, typename Context>
  void applyTo(Storage &data, Context &target);
  template <typename Storage>
  void dumpLog(Storage &data) const;
};

struct StateFieldGraphicsRenderPassClass : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericPtr<RenderPassClass>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsBasePipeline : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericPtr<VariatedGraphicsPipeline>
{
  void resolveResourceBarriers(BackGraphicsStateStorage &storage, ExecutionContext &target);

  VULKAN_TRACKED_STATE_FIELD_CB_NON_CONST_DEFENITIONS();
};

struct StateFieldGraphicsPipelineLayout : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericPtr<GraphicsPipelineLayout>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsPipeline : TrackedStateFieldBase<true, false>
{
  typedef uint32_t Invalidate;
  // for cases when pipeline need to be fully rebound on new command buffer
  struct FullInvalidate
  {};

  GraphicsPipeline *ptr;
  GraphicsPipeline *oldPtr;

  template <typename StorageType>
  void reset(StorageType &)
  {
    ptr = nullptr;
    oldPtr = nullptr;
  }

  VULKAN_TRACKED_STATE_FIELD_CB_NON_CONST_DEFENITIONS();

  void set(Invalidate)
  {
    if (ptr)
      oldPtr = ptr;
    ptr = nullptr;
  }
  bool diff(Invalidate) { return true; }

  void set(FullInvalidate)
  {
    ptr = nullptr;
    oldPtr = nullptr;
  }
  bool diff(FullInvalidate) { return true; }

  void set(GraphicsPipeline *value)
  {
    oldPtr = ptr;
    ptr = value;
  }
  bool diff(GraphicsPipeline *value) const { return ptr != value; }
};

struct StateFieldGraphicsFramebuffer : TrackedStateFieldBase<true, false>,
                                       TrackedStateFieldGenericWrappedHandle<VulkanFramebufferHandle>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsRenderPassScopeOpener : TrackedStateFieldBase<false, false>, TrackedStateFieldGenericSmallPOD<bool>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsRenderPassEarlyScopeOpener : TrackedStateFieldBase<false, false>, TrackedStateFieldGenericSmallPOD<bool>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

enum class InPassStateFieldType
{
  NONE,
  NORMAL_PASS,
  NATIVE_PASS
};

struct StateFieldGraphicsInPass : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericSmallPOD<InPassStateFieldType>
{
  InPassStateFieldType &getValue() { return data; };

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsScissorRect : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericPOD<VkRect2D>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsScissor : TrackedStateFieldBase<true, false>
{
  bool enabled;
  VkRect2D rect;
  VkRect2D viewRect;

  struct MakeDirty
  {
    uint32_t dummy;
  };

  void reset(BackDynamicGraphicsStateStorage &)
  {
    rect = {};
    viewRect = {};
    enabled = false;
  }

  void set(const VkRect2D &value) { rect = value; }
  void set(bool value) { enabled = value; }
  void set(const ViewportState &vp) { viewRect = vp.rect2D; }
  void set(const MakeDirty &) {}

  bool diff(const VkRect2D &value) { return value != rect; }
  bool diff(bool value) { return value != enabled; }
  bool diff(const ViewportState &vp) { return viewRect != vp.rect2D; }
  bool diff(const MakeDirty &) { return true; }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsDepthBias : TrackedStateFieldBase<true, false>
{
  struct DataType
  {
    float bias;
    float slopedBias;
  };

  DataType data;

  void reset(BackDynamicGraphicsStateStorage &) { data = {}; }
  void set(const DataType &val) { data = val; }
  bool diff(const DataType &val) { return (data.bias != val.bias) || (data.slopedBias != val.slopedBias); }
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsDepthBounds : TrackedStateFieldBase<true, false>
{
  struct DataType
  {
    float min;
    float max;
  };

  DataType data;

  template <typename Storage>
  void reset(Storage &)
  {
    data = {};
  }

  void set(const DataType &val) { data = val; }
  bool diff(const DataType &val) { return (data.min != val.min) || (data.max != val.max); }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsStencilRef : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericSmallPOD<uint8_t>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsStencilRefOverride : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericSmallPOD<uint16_t>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsStencilMask : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericSmallPOD<uint8_t>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsDynamicRenderStateIndex : TrackedStateFieldBase<true, false>,
                                                   TrackedStateFieldGenericSmallPOD<LinearStorageIndex>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsBlendConstantFactor : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericPOD<E3DCOLOR>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsIndexBuffer : TrackedStateFieldBase<true, false>
{
  BufferRef data;
  VkIndexType indexType;

  template <typename T>
  void reset(T &)
  {
    data = BufferRef{};
    indexType = VK_INDEX_TYPE_UINT16;
  }

  void set(const BufferRef &value) { data = value; }
  void set(VkIndexType value) { indexType = value; }
  void set(uint32_t) {}

  bool diff(const BufferRef &value) { return data != value; }
  bool diff(VkIndexType value) { return indexType != value; }
  bool diff(uint32_t val) { return val != 0; }

  BufferRef &getValue() { return data; }
  const BufferRef &getValueRO() const { return data; }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsViewport : TrackedStateFieldBase<true, false>
{
  struct RestoreFromFramebuffer
  {
    uint32_t dummy;
  };
  struct MakeDirty
  {
    uint32_t dummy;
  };

  ViewportState data;

  template <typename T>
  void reset(T &)
  {
    data = {};
  }

  void set(const ViewportState &value) { data = value; }
  bool diff(const ViewportState &value) { return data != value; }

  void set(const RestoreFromFramebuffer &);
  bool diff(const RestoreFromFramebuffer &) { return true; }

  void set(const MakeDirty &) {}
  bool diff(const MakeDirty &) { return true; }

  void set(const RenderPassArea &value)
  {
    data.rect2D.offset.x = value.left;
    data.rect2D.offset.y = value.top;
    data.rect2D.extent.width = value.width;
    data.rect2D.extent.height = value.height;
    data.maxZ = value.maxZ;
    data.minZ = value.minZ;
  }
  bool diff(const RenderPassArea &) { return true; }

  const ViewportState &getValueRO() const { return data; }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsVertexBufferBind
{
  using Indexed = TrackedStateIndexedDataRORef<StateFieldGraphicsVertexBufferBind>;

  BufferRef bRef;
  uint32_t offset;

  template <typename T>
  void reset(T &)
  {
    bRef = BufferRef();
    offset = 0;
  }

  void set(const StateFieldGraphicsVertexBufferBind &v);
  bool diff(const StateFieldGraphicsVertexBufferBind &v) const;

  VULKAN_TRACKED_STATE_ARRAY_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsVertexBuffers
  : TrackedStateFieldArray<StateFieldGraphicsVertexBufferBind, MAX_VERTEX_INPUT_STREAMS, true, true>
{};

// backend field for optimized vkCmd* calling
struct StateFieldGraphicsVertexBuffersBindArray : TrackedStateFieldBase<false, false>
{
  Buffer *resPtrs[MAX_VERTEX_INPUT_STREAMS];
  VulkanBufferHandle buffers[MAX_VERTEX_INPUT_STREAMS];
  VkDeviceSize offsets[MAX_VERTEX_INPUT_STREAMS];
  uint32_t countMask;

  template <typename StorageType>
  void reset(StorageType &)
  {
    // do not waste time on memset buffers, just avoid using items that was not bit-marked
    countMask = 0;
  }

  void set(const BufferSubAllocation &bsa)
  {
    resPtrs[0] = bsa.buffer;
    buffers[0] = bsa.buffer->getHandle();
    offsets[0] = bsa.buffer->dataOffset(bsa.offset);
    countMask |= 1 << 0;
  }
  bool diff(const BufferSubAllocation &) const { return true; }

  void set(const StateFieldGraphicsVertexBufferBind::Indexed &bind)
  {
    if (bind.val.bRef)
    {
      resPtrs[bind.index] = bind.val.bRef.buffer;
      buffers[bind.index] = bind.val.bRef.getHandle();
      offsets[bind.index] = bind.val.bRef.dataOffset(bind.val.offset);
      countMask |= 1 << bind.index;
    }
    else
      countMask &= ~(1 << bind.index);
  }
  bool diff(const StateFieldGraphicsVertexBufferBind::Indexed &) const { return true; }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsVertexBufferStride : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericSmallPOD<uint32_t>
{
  using Indexed = TrackedStateIndexedDataRORef<uint32_t>;
  VULKAN_TRACKED_STATE_ARRAY_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsVertexBufferStrides
  : TrackedStateFieldArray<StateFieldGraphicsVertexBufferStride, MAX_VERTEX_INPUT_STREAMS, true, true>
{};

struct StateFieldGraphicsPrimitiveTopology : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericSmallPOD<VkPrimitiveTopology>
{
  template <typename StorageType>
  void reset(StorageType &)
  {
    data = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

// TODO: remove this when all data is properly tracked
struct StateFieldGraphicsFlush : TrackedStateFieldBase<false, false>
{
  bool needPipeline = false;

  template <typename StorageType>
  void reset(StorageType &)
  {
    needPipeline = false;
  }
  void set(uint32_t v) { needPipeline = v != 0; }
  bool diff(uint32_t) { return true; }
  const bool &getValueRO() const { return needPipeline; }

  void applyDescriptors(BackGraphicsStateStorage &state, ExecutionContext &target) const;
  void applyBarriers(BackGraphicsStateStorage &state, ExecutionContext &target) const;
  void syncTLayoutsToRenderPass(BackGraphicsStateStorage &state, ExecutionContext &target) const;

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct GraphicsQueryState
{
  using This = GraphicsQueryState;
  struct InvalidateTag
  {};

  VulkanQueryPoolHandle pool{};
  uint32_t index{0};

  friend bool operator==(const This &left, const This &right) { return left.pool == right.pool && left.index == right.index; }

  friend bool operator!=(const This &left, const This &right) { return !(left == right); }
};

struct StateFieldGraphicsQueryState : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericPOD<GraphicsQueryState>
{
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldGraphicsQueryScopeOpener : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericPOD<GraphicsQueryState>
{
  // Bring parent's function into this scope so that they get included
  // into the overload sets on name resolution (otherwise they would not
  // get considered at all...).
  using TrackedStateFieldGenericPOD<GraphicsQueryState>::set;
  using TrackedStateFieldGenericPOD<GraphicsQueryState>::diff;

  void set(GraphicsQueryState::InvalidateTag) {}
  bool diff(GraphicsQueryState::InvalidateTag) const { return true; }

  template <typename Storage, typename Context>
  void applyTo(Storage &data, Context &target);
  template <typename Storage>
  void dumpLog(Storage &data) const;
};

struct StateFieldGraphicsQueryScopeCloser : TrackedStateFieldBase<true, false>
{
  GraphicsQueryState data;
  ActiveExecutionStage stage;
  bool shouldClose;

  template <typename StorageType>
  void reset(StorageType &)
  {
    data = {};
    stage = ActiveExecutionStage::FRAME_BEGIN;
    shouldClose = false;
  }

  void set(const ActiveExecutionStage to) { stage = to; }
  bool diff(const ActiveExecutionStage) const { return true; }

  void set(const GraphicsQueryState &value) { data = value; }
  bool diff(const GraphicsQueryState &value) const { return data != value; }

  void set(GraphicsQueryState::InvalidateTag) {}
  bool diff(GraphicsQueryState::InvalidateTag) const { return shouldClose; }

  template <typename Storage, typename Context>
  void applyTo(Storage &data, Context &target);
  template <typename Storage>
  void dumpLog(Storage &data) const;
};

struct StateFieldGraphicsRenderPassArea : TrackedStateFieldBase<true, false>, TrackedStateFieldGenericPOD<VkRect2D>
{
  const VkRect2D &getValueRO() const { return data; };
  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

} // namespace drv3d_vulkan
