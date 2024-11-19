// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "image_view_state.h"
#include "sampler_state.h"

#include <dag/dag_vector.h>
#include <drv/3d/dag_driver.h>
#include <generic/dag_variantVector.h>
#include <osApiWrappers/dag_critSec.h>
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
class BindlessManager
{
  struct BufferSlotUsage
  {
    Sbuffer *buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
  };
  struct TextureSlotUsage
  {
    BaseTex *texture;
    Image *image;
    ImageViewState view;
  };

  struct State
  {
    dag::Vector<ValueRange<uint32_t>> freeSlotRanges;
    dag::VariantVector<eastl::monostate, BufferSlotUsage, TextureSlotUsage> resourceSlotInfo;
    dag::Vector<SamplerState> samplerTable;
  };

  State state;
  const NullResourceTable *nullResourceTable = nullptr;

public:
  void init(DeviceContext &ctx, SamplerDescriptorAndState default_bindless_sampler, const NullResourceTable *null_resource_table);

  uint32_t registerSampler(Device &device, DeviceContext &ctx, BaseTex *texture);
  uint32_t registerSampler(DeviceContext &ctx, SamplerDescriptorAndState sampler);

  uint32_t allocateBindlessResourceRange(uint32_t count);
  uint32_t resizeBindlessResourceRange(DeviceContext &ctx, uint32_t index, uint32_t current_count, uint32_t new_count);

  void freeBindlessResourceRange(uint32_t index, uint32_t count);
  void updateBindlessBuffer(DeviceContext &ctx, uint32_t index, Sbuffer *buffer);
  void updateBindlessTexture(DeviceContext &ctx, uint32_t index, BaseTex *res);

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
    uint32_t change_max_count, bool ignore_previous_view);
  void updateBufferReferencesNoLock(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE old_descriptor,
    D3D12_CPU_DESCRIPTOR_HANDLE new_descriptor);

  bool hasTextureReference(BaseTex *tex);
  bool hasTextureReferenceNoLock(BaseTex *tex);
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

  void updateBindlessNull(DeviceContext &ctx, uint32_t resource_type, uint32_t index, uint32_t count);
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

private:
  static ImageViewState adjust_previous_view(ImageViewState previous_view, int old_count, int new_count);
  template <typename SlotType, typename CheckerType>
  void resetReferences(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE nullDescriptor, const CheckerType &checker);
  template <typename SlotType, typename CheckerType>
  void resetReferencesNoLock(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE nullDescriptor, const CheckerType &checker);

  uint32_t allocateBindlessResourceRangeNoLock(uint32_t count);
  void freeBindlessResourceRangeNoLock(uint32_t index, uint32_t count);

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
  // This is supposed to be called after descriptor space for bindless was reserved (this->reserveResourceHeap),
  // all regular SRV and UAV descriptors where pushed and migrated to the active heap.
  // This will then write the current bindless heap revision to the active descriptor heap.
  void pushToResourceHeap(ID3D12Device *device, ShaderResourceViewDescriptorHeapManager &target);

  void setSamplerDescriptor(ID3D12Device *device, uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
  void reserveSamplerHeap(ID3D12Device *device, SamplerDescriptorHeapManager &target);
  void pushToSamplerHeap(ID3D12Device *device, SamplerDescriptorHeapManager &target);

  void copyResourceDescriptors(ID3D12Device *device, uint32_t src, uint32_t dst, uint32_t count);
  uint32_t getResourceDescriptorCount() const { return resourceDescriptorSize; }
  uint32_t getResourceDescriptorRevision() const { return resourceDescriptorHeapRevision; }
};
} // namespace backend
} // namespace drv3d_dx12