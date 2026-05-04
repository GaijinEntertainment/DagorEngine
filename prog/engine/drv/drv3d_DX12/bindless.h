// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "image_view_state.h"
#include "sampler_state.h"
#include "container_mutex_wrapper.h"

#include <dag/dag_vector.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_bindless.h>
#include <EASTL/optional.h>
#include <generic/dag_variantVector.h>
#include <generic/dag_span.h>
#include <osApiWrappers/dag_spinlock.h>
#include <supp/dag_comPtr.h>


namespace drv3d_dx12
{

class Device;
class DeviceContext;

class BaseTex;
class Image;
class ShaderResourceViewDescriptorHeapManager;
class SamplerDescriptorHeapManager;

struct NullResourceTable;

namespace frontend
{
class BindlessTypesValidatorImplementation
{
public:
  void setTypesRange(D3DResourceType type, uint32_t index, uint32_t size);
  void resetTypesRange(uint32_t index, uint32_t size);
  void checkTypesRange(D3DResourceType type, uint32_t index, uint32_t size) const;
  void enableTypesValidation(bool is_enabled) { isEnabled = is_enabled; }
  void resetTypesValidation() { resourceTypes.clear(); }

private:
  dag::Vector<eastl::optional<D3DResourceType>> resourceTypes;
  bool isEnabled = false;
};

class BindlessTypesValidatorStubImplementation
{
public:
  void setTypesRange(D3DResourceType, uint32_t, uint32_t) {}
  void resetTypesRange(uint32_t, uint32_t) {}
  void checkTypesRange(D3DResourceType, uint32_t, uint32_t) const {}
  void enableTypesValidation(bool) {}
  void resetTypesValidation() {}

private:
};

#if DX12_VALIDATE_BINDLESS_TYPES
using BindlessTypesValidator = BindlessTypesValidatorImplementation;
#else
using BindlessTypesValidator = BindlessTypesValidatorStubImplementation;
#endif

class BindlessManager : BindlessTypesValidator
{
  struct BufferSlotUsage
  {
    Sbuffer *buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
    void reset() { descriptor = D3D12_CPU_DESCRIPTOR_HANDLE(); }
  };
  struct TextureSlotUsage
  {
    BaseTex *texture;
    Image *image;
    ImageViewState view;
    void reset() { image = nullptr; }
  };

  struct State
  {
    dag::Vector<ValueRange<uint32_t>> freeSlotRanges;
    dag::VariantVector<eastl::monostate, BufferSlotUsage, TextureSlotUsage> resourceSlotInfo;
    SamplerState samplerTable[::bindless::MAX_SAMPLER_INDEX_COUNT];
    uint32_t samplerTableAllocated = 0;
    ContainerMutexWrapper<dag::Vector<D3D12_CPU_DESCRIPTOR_HANDLE>, OSSpinlock> pendingBufferFrees;
    ContainerMutexWrapper<dag::Vector<BaseTex *>, OSSpinlock> pendingTextureFrees;
  };

  State state;
  const NullResourceTable *nullResourceTable = nullptr;

public:
  void init(DeviceContext &ctx, SamplerDescriptorAndState default_bindless_sampler, const NullResourceTable *null_resource_table,
    bool enable_types_validation);

  uint32_t registerSampler(Device &device, DeviceContext &ctx, BaseTex *texture);
  uint32_t registerSampler(DeviceContext &ctx, SamplerDescriptorAndState sampler);

  uint32_t allocateBindlessResourceRange(D3DResourceType type, uint32_t count);
  uint32_t resizeBindlessResourceRange(D3DResourceType type, DeviceContext &ctx, uint32_t index, uint32_t current_count,
    uint32_t new_count);

  void freeBindlessResourceRange(D3DResourceType type, uint32_t index, uint32_t count);
  bool updateBindlessBuffer(DeviceContext &ctx, uint32_t index, Sbuffer *buffer);
  void updateBindlessBufferRange(DeviceContext &ctx, D3DResourceType type, uint32_t index, const dag::Span<Sbuffer *> &buffers);
  bool updateBindlessTexture(DeviceContext &ctx, uint32_t index, BaseTex *res);
  void updateBindlessTextureRange(DeviceContext &ctx, D3DResourceType type, uint32_t index, const dag::Span<BaseTex *> &textures);

  struct CheckTextureImagePairResultType
  {
    uint32_t firstFound = 0;
    // Only valid if missmatchFound is false. IF mismatchFound is false and matchCount is 0, none was found.
    uint32_t matchCount = 0;
    bool mismatchFound = false;
  };
  // Checks if for all entries for tex it uses image as image. This may be false when the engine is currently
  // changing internals of tex to use a new image.
  // Requires front guard and binding guard locked.
  CheckTextureImagePairResultType checkTextureImagePairNoLock(BaseTex *tex, Image *image);
  // Values for search_offset and change_count can be obtained with checkTextureImagePair, where
  // CheckTextureImagePairResultType::firstFound can be used for search_offset and
  // CheckTextureImagePairResultType::matchCount can be used for change_count.
  // Default values can be 0 for search_offset and ~0 for change_count, the method will still
  // function as expected by searching the whole set and replacing all found matches.
  // Requires front guard and binding guard locked.
  void updateTextureReferencesNoLock(DeviceContext &ctx, BaseTex *tex, Image *old_image, uint32_t search_offset,
    uint32_t change_max_count, bool deferred = false);
  bool updateBufferReferencesNoLock(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE old_descriptor,
    D3D12_CPU_DESCRIPTOR_HANDLE new_descriptor);

  bool hasValidTextureReference(BaseTex *tex);
  bool hasValidTextureReferenceNoLock(BaseTex *tex);
  bool hasTextureImageReference(BaseTex *tex, Image *image);
  bool hasTextureImageViewReference(BaseTex *tex, Image *image, ImageViewState view);
  bool hasImageReference(Image *image);
  bool hasImageViewReference(Image *image, ImageViewState view);
  bool hasBufferReference(Sbuffer *buf);
  bool hasBufferViewReference(Sbuffer *buf, D3D12_CPU_DESCRIPTOR_HANDLE view);
  bool hasBufferViewReference(D3D12_CPU_DESCRIPTOR_HANDLE view);

  void resetTextureReferences(DeviceContext &ctx, BaseTex *texture);
  void resetTextureImagePairReferencesNoLock(DeviceContext &ctx, BaseTex *texture, Image *image);
  void resetBufferReferences(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

  void updateBindlessNull(DeviceContext &ctx, D3DResourceType type, uint32_t index, uint32_t count);
  void preRecovery();

  template <typename T>
  void visitSamplers(T clb)
  {
    OSSpinlockScopedLock stateLock{get_state_guard()};

    for (auto &&s : state.samplerTable)
    {
      clb(s);
    }
  }

  void completeFrameRecording(DeviceContext &ctx);
  void onBufferFree(D3D12_CPU_DESCRIPTOR_HANDLE buffer_descriptor);
  void onTextureFree(BaseTex *texture);

  void reportBindlessSlotsInfo();
  void reportBindlessSlotsAllocationInfo();

private:
  template <typename SlotType, typename CheckerType>
  void resetReferences(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE nullDescriptor, const CheckerType &checker);
  template <typename SlotType, typename CheckerType>
  void resetReferencesNoLock(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE nullDescriptor, const CheckerType &checker);
  template <typename SlotType, typename CheckerType>
  void removeReferences(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE nullDescriptor, const CheckerType &checker);
  template <typename SlotType, typename CheckerType>
  void removeReferencesNoLock(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE nullDescriptor, const CheckerType &checker);

  void reportBindlessSlotsInfoNoLock();
  void reportBindlessSlotsAllocationInfoNoLock();
  uint32_t allocateBindlessResourceRangeNoLock(uint32_t count);
  void freeBindlessResourceRangeNoLock(D3DResourceType type, uint32_t index, uint32_t count);
  void copyBindlessResourceRangeNoLock(uint32_t src, uint32_t dst, uint32_t count);

  void checkBindlessRangeAllocatedNoLock(uint32_t index, uint32_t count) const;

  static OSSpinlock &get_state_guard();
};
} // namespace frontend
namespace backend
{
class BindlessSetManager
{
  ComPtr<ID3D12DescriptorHeap> resourceDescriptorHeap;
  ComPtr<ID3D12DescriptorHeap> samplerHeap;
  D3D12_CPU_DESCRIPTOR_HANDLE resourceDescriptorStart{};
  D3D12_CPU_DESCRIPTOR_HANDLE samplerHeapStart{};
  dag::Vector<eastl::pair<uint32_t, D3D12_CPU_DESCRIPTOR_HANDLE>> deferredResourceDescriptors;

  uint32_t resourceDescriptorSize = 0;
  uint32_t resourceDescriptorHeapRevision = 0;
  uint32_t samplerDescriptorSize = 0;
  uint32_t resourceDescriptorWidth = 0;
  uint32_t samplerDescriptorWidth = 0;

public:
  void init(ID3D12Device *device);
  void shutdown();
  void preRecovery();
  void recover(ID3D12Device *device, const dag::Vector<D3D12_CPU_DESCRIPTOR_HANDLE> &bindless_samplers);

  void setResourceDescriptor(ID3D12Device *device, uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
  // Defers a resource descriptor update to be applied after the current frame's command list is submitted.
  // Used by defragmentation moves to avoid updating the GPU-visible descriptor heap for the current frame.
  void deferResourceDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
  // Applies all deferred resource descriptor updates to the staging heap. Should be called after command list submission.
  void applyDeferredResourceDescriptors(ID3D12Device *device);
  // This is supposed to be called after descriptor space for bindless was reserved (this->reserveResourceHeap),
  // all regular SRV and UAV descriptors where pushed and migrated to the active heap.
  // This will then write the current bindless heap revision to the active descriptor heap.
  void pushToResourceHeap(ID3D12Device *device, ShaderResourceViewDescriptorHeapManager &target);

  void setSamplerDescriptor(ID3D12Device *device, uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
  void pushToSamplerHeaps(ID3D12Device *device, SamplerDescriptorHeapManager &target);

  void copyResourceDescriptors(ID3D12Device *device, uint32_t src, uint32_t dst, uint32_t count);
  uint32_t getResourceDescriptorCount() const { return resourceDescriptorSize; }
  uint32_t getResourceDescriptorRevision() const { return resourceDescriptorHeapRevision; }
};
} // namespace backend
} // namespace drv3d_dx12