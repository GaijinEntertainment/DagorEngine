#pragma once

#include <3d/dag_drv3d.h>
#include <EASTL/vector.h>
#include <EASTL/variant.h>
#include <EASTL/span.h>
#include <osApiWrappers/dag_critSec.h>
#include <supp/dag_comPtr.h>

#include "image_view_state.h"
#include "sampler_state.h"
#include "container_mutex_wrapper.h"


namespace drv3d_dx12
{

class Device;
class DeviceContext;

struct BaseTex;
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
  using ResourceSlotInfoType = eastl::variant<eastl::monostate, BufferSlotUsage, TextureSlotUsage>;

  struct State
  {
    eastl::vector<ValueRange<uint32_t>> freeSlotRanges;
    eastl::vector<ResourceSlotInfoType> resourceSlotInfo;
    eastl::vector<SamplerState> samplerTable;
  };

  ContainerMutexWrapper<State, WinCritSec> state;

public:
  uint32_t registerSampler(Device &device, DeviceContext &ctx, BaseTex *texture);
  void unregisterBufferDescriptors(eastl::span<const D3D12_CPU_DESCRIPTOR_HANDLE> descriptors);

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
  CheckTextureImagePairResultType checkTextureImagePair(BaseTex *tex, Image *image);
  // Values for search_offset and change_count can be obtained with checkTextureImagePair, where
  // CheckTextureImagePairResultType::firstFound can be used for search_offset and
  // CheckTextureImagePairResultType::matchCount can be used for change_count.
  // Default values can be 0 for search_offset and ~0 for change_count, the method will still
  // function as expected by searching the whole set and replacing all found matches.
  void updateTextureReferences(DeviceContext &ctx, BaseTex *tex, Image *old_image, Image *new_image, uint32_t search_offset,
    uint32_t change_count);
  void updateBufferReferences(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE old_descriptor,
    D3D12_CPU_DESCRIPTOR_HANDLE new_descriptor);

  bool hasTextureReference(BaseTex *tex);
  bool hasTextureImageReference(BaseTex *tex, Image *image);
  bool hasTextureImageViewReference(BaseTex *tex, Image *image, ImageViewState view);
  bool hasImageReference(Image *image);
  bool hasImageViewReference(Image *image, ImageViewState view);
  bool hasBufferReference(Sbuffer *buf);
  bool hasBufferViewReference(Sbuffer *buf, D3D12_CPU_DESCRIPTOR_HANDLE view);

  void onTextureDeletion(DeviceContext &ctx, BaseTex *texture, const NullResourceTable &null_table);
  void updateBindlessNull(DeviceContext &ctx, uint32_t resource_type, uint32_t index, uint32_t count,
    const NullResourceTable &null_table);
  void preRecovery();

  template <typename T>
  void visitSamplers(T clb)
  {
    auto stateAccess = state.access();
    for (auto &&s : stateAccess->samplerTable)
    {
      clb(s);
    }
  }
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
  void recover(ID3D12Device *device, const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> &bindless_samplers);

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