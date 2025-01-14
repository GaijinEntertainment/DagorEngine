// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resource_memory_heap.h"
#include "frontend_state.h"

#include <supp/dag_cpuControl.h>
#include <3d/dag_resourceDump.h>


using namespace drv3d_dx12;
using namespace drv3d_dx12::resource_manager;

ImageCreateResult TextureImageFactory::createTexture(DXGIAdapter *adapter, Device &device, const ImageInfo &ii, const char *name)
{
  ImageCreateResult result{};
  auto desc = ii.asDesc();

  result.state = propertiesToInitialState(desc.Dimension, desc.Flags, ii.memoryClass);

  HRESULT errorCode = S_OK;

  if (ii.memoryClass != DeviceMemoryClass::RESERVED_RESOURCE)
  {
    auto allocInfo = get_resource_allocation_info(device.getDevice(), desc);
    if (!is_valid_allocation_info(allocInfo))
    {
      report_resource_alloc_info_error(desc);
      return result;
    }
    auto memoryProperties = getProperties(desc.Flags, ii.memoryClass, allocInfo.Alignment);

    AllocationFlags allocationFlags;
#if _TARGET_PC_WIN
    // enable fallback to committed resources only on devices with Resource Heap Tier 1
    if (!canMixResources())
      allocationFlags = AllocationFlag::NEW_HEAPS_ONLY_WITH_BUDGET;
#endif
    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
      allocationFlags.set(AllocationFlag::IS_UAV);
    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
      allocationFlags.set(AllocationFlag::IS_RTV);

    auto oomCheckOnExit = checkForOOMOnExit(
      adapter, [&errorCode]() { return !is_oom_error_code(errorCode); },
      OomReportData{"createTexture", name, allocInfo.SizeInBytes, allocationFlags.to_ulong(), memoryProperties.raw});

    auto memory = allocate(adapter, device.getDevice(), memoryProperties, allocInfo, allocationFlags, &errorCode);

    if (!memory)
    {
      device.processEmergencyDefragmentation(memoryProperties.raw, true, allocationFlags.test(AllocationFlag::IS_UAV),
        allocationFlags.test(AllocationFlag::IS_RTV));
      errorCode = S_OK;
      memory = allocate(adapter, device.getDevice(), memoryProperties, allocInfo, allocationFlags, &errorCode);
    }

    if (!memory)
    {
#if _TARGET_PC_WIN
      ComPtr<ID3D12Resource> texture;

      D3D12_HEAP_PROPERTIES heapProperties{};
      heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
      heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
      heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
      heapProperties.CreationNodeMask = 0;
      heapProperties.VisibleNodeMask = 0;

      D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;

      errorCode = DX12_CHECK_RESULT_NO_OOM_CHECK(
        device.getDevice()->CreateCommittedResource(&heapProperties, heapFlags, &desc, result.state, nullptr, COM_ARGS(&texture)));

      if (!texture)
      {
        logwarn("DX12: Unable to allocate committed resource");
        return result;
      }

      const ImageGlobalSubresourceId subResIdBase = (ii.allocateSubresourceIDs)
                                                      ? allocateGlobalResourceIdRange(ii.getSubResourceCount())
                                                      : ImageGlobalSubresourceId::make_invalid();

      result.image = newImageObject(ResourceMemory{}, eastl::move(texture), ii.type, ii.memoryLayout, ii.format, ii.size, ii.mips,
        ii.arrays, subResIdBase, get_log2i_of_pow2(ii.sampleDesc.Count));

      recordTextureAllocated(result.image->getMipLevelRange(), result.image->getArrayLayers(), result.image->getBaseExtent(),
        result.image->getMemory().size(), result.image->getFormat(), name);

      // manually increase memory pool usage and trigger update at frame end
      poolStates[device_local_memory_pool].CurrentUsage += allocInfo.SizeInBytes;
      behaviorStatus.reset(BehaviorBits::DISABLE_DEVICE_MEMORY_STATUS_QUERY);
#else
      // alloc failed, allocator will complain about this so no need to repeat it
      return result;
#endif
    }
    else
    {
      ComPtr<ID3D12Resource> texture;
      errorCode =
#if _TARGET_XBOX
        DX12_CHECK_RESULT_NO_OOM_CHECK(
          xbox_create_placed_resource(device.getDevice(), memory.getAddress(), desc, result.state, nullptr, texture));
#else
        DX12_CHECK_RESULT_NO_OOM_CHECK(device.getDevice()->CreatePlacedResource(memory.getHeap(), memory.getOffset(), &desc,
          result.state, nullptr, COM_ARGS(&texture)));
#endif

      if (!texture)
      {
        free(memory, false);
        return result;
      }

      const ImageGlobalSubresourceId subResIdBase = (ii.allocateSubresourceIDs)
                                                      ? allocateGlobalResourceIdRange(ii.getSubResourceCount())
                                                      : ImageGlobalSubresourceId::make_invalid();

      result.image = newImageObject(memory, eastl::move(texture), ii.type, ii.memoryLayout, ii.format, ii.size, ii.mips, ii.arrays,
        subResIdBase, get_log2i_of_pow2(ii.sampleDesc.Count));

      updateMemoryRangeUse(memory, result.image);
      recordTextureAllocated(result.image->getMipLevelRange(), result.image->getArrayLayers(), result.image->getBaseExtent(),
        result.image->getMemory().size(), result.image->getFormat(), name);
    }
  }
  else
  {
    auto oomCheckOnExit = checkForOOMOnExit(
      adapter, [&errorCode]() { return !is_oom_error_code(errorCode); }, OomReportData{"createTexture (reserved)", name});


    ComPtr<ID3D12Resource> texture;
    errorCode =
      DX12_CHECK_RESULT_NO_OOM_CHECK(device.getDevice()->CreateReservedResource(&desc, result.state, nullptr, COM_ARGS(&texture)));

    if (!texture)
    {
      return result;
    }

    const ImageGlobalSubresourceId subResIdBase =
      (ii.allocateSubresourceIDs) ? allocateGlobalResourceIdRange(ii.getSubResourceCount()) : ImageGlobalSubresourceId::make_invalid();

    result.image = newImageObject(ResourceMemory{}, eastl::move(texture), ii.type, ii.memoryLayout, ii.format, ii.size, ii.mips,
      ii.arrays, subResIdBase, get_log2i_of_pow2(ii.sampleDesc.Count));
  }

  result.image->setGPUChangeable(
    0 != (ii.usage & (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL |
                       D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)));

  return result;
}

Image *TextureImageFactory::adoptTexture(ID3D12Resource *texture, const char *name)
{
  ComPtr<ID3D12Resource> texture_ref{texture};

  // TODO: this works only 2D
  D3D12_RESOURCE_DESC desc = texture->GetDesc();
  G_ASSERT(D3D12_RESOURCE_DIMENSION_BUFFER != desc.Dimension);
  G_ASSERT(D3D12_RESOURCE_DIMENSION_TEXTURE3D != desc.Dimension);
  FormatStore dagorFormat = (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) ? FormatStore::fromDXGIDepthFormat(desc.Format)
                                                                                   : FormatStore::fromDXGIFormat(desc.Format);
  const auto idCount = ArrayLayerCount::make(desc.DepthOrArraySize) * MipMapCount::make(desc.MipLevels) * dagorFormat.getPlanes();

  auto subResIdBase = allocateGlobalResourceIdRange(idCount);
  auto result = newImageObject(ResourceMemory{}, eastl::move(texture_ref), D3D12_RESOURCE_DIMENSION_TEXTURE2D, desc.Layout,
    dagorFormat, Extent3D{uint32_t(desc.Width), uint32_t(desc.Height), 1}, MipMapCount::make(desc.MipLevels),
    ArrayLayerCount::make(desc.DepthOrArraySize), subResIdBase, 0);

  recordTextureAdopted(result->getMipLevelRange(), result->getArrayLayers(), result->getBaseExtent(), result->getFormat(), name);

  return result;
}

// assumes mutex is locked
void TextureImageFactory::freeView(const ImageViewInfo &view)
{
  if (view.state.isSRV() || view.state.isUAV())
  {
    freeTextureSRVDescriptor(view.handle);
  }
  else if (view.state.isRTV())
  {
    freeTextureRTVDescriptor(view.handle);
  }
  else if (view.state.isDSV())
  {
    freeTextureDSVDescriptor(view.handle);
  }
}

Image *TextureImageFactory::tryCloneTexture(DXGIAdapter *adapter, ID3D12Device *device, Image *original,
  D3D12_RESOURCE_STATES initial_state, AllocationFlags allocation_flags)
{
  Image *result = nullptr;
  auto desc = original->getHandle()->GetDesc();
  // On xbox GetDesc can return 0 if resource has default alignment. So we should adjust it here.
  // GDK Documentation says: If Alignment is set to 0, the runtime will use 4MB for MSAA textures and 64KB for everything else.
  // Despite the fact that here we know alignment here, we use calculate_texture_alignment just in case.
  desc.Alignment = calculate_texture_alignment(desc.Width, desc.Height, original->getBaseExtent().depth, desc.SampleDesc.Count,
    desc.Layout, desc.Flags, original->getFormat());
  auto allocInfo = get_resource_allocation_info(device, desc);
  if (!is_valid_allocation_info(allocInfo))
  {
    report_resource_alloc_info_error(desc);
    return result;
  }
  auto properties = getHeapProperties(original->getMemory().getHeapID());
  auto memory = allocate(adapter, device, properties, allocInfo, allocation_flags);

  if (!memory)
  {
    // alloc failed, allocator will complain about this so no need to repeat it
    return result;
  }

  ComPtr<ID3D12Resource> texture;
#if _TARGET_XBOX
  DX12_CHECK_RESULT(device->CreatePlacedResourceX(memory.getAddress(), &desc, initial_state, nullptr, COM_ARGS(&texture)));
#else
  DX12_CHECK_RESULT(
    device->CreatePlacedResource(memory.getHeap(), memory.getOffset(), &desc, initial_state, nullptr, COM_ARGS(&texture)));
#endif

  if (!texture)
  {
    free(memory, allocation_flags.test(AllocationFlag::DEFRAGMENTATION_OPERATION));
    return result;
  }

  auto subResIdBase = original->hasTrackedState()
                        ? allocateGlobalResourceIdRange(original->getSubresourcesPerPlane() * original->getPlaneCount())
                        : ImageGlobalSubresourceId::make_invalid();
  result = newImageObject(memory, eastl::move(texture), original->getType(), D3D12_TEXTURE_LAYOUT_UNKNOWN, original->getFormat(),
    original->getBaseExtent(), original->getMipLevelRange(), original->getArrayLayers(), subResIdBase, original->getMsaaLevel());

  original->getDebugName([=](const auto &name) { result->setDebugName(name); });

  updateMemoryRangeUse(memory, result);
  result->getDebugName([=](const auto &name) {
    recordTextureAllocated(result->getMipLevelRange(), result->getArrayLayers(), result->getBaseExtent(), result->getMemory().size(),
      result->getFormat(), name);
  });
  result->setGPUChangeable(0 != (desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL |
                                                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)));

  return result;
}

void TextureImageFactory::destroyTextures(eastl::span<Image *> textures, frontend::BindlessManager &bindless_manager)
{
  G_UNUSED(bindless_manager);
  for (auto texture : textures)
  {
    if (texture->getGlobalSubResourceIdBase().isValid())
    {
      auto idrange = texture->getGlobalSubresourceIdRange();
      freeGlobalResourceIdRange(ValueRange<ImageGlobalSubresourceId>(idrange.start, idrange.stop));
    }
    if (texture->isHeapAllocated() && !texture->isAliased())
    {
      free(texture->getMemory(), false);
    }
    for (auto &view : texture->getOldViews())
    {
      freeView(view);
    }
    freeView(texture->getRecentView());
    texture->getDebugName([this, texture](auto &name) {
      logdbg("DX12: Destroy image: %p (handle: %p, name: %s)", texture, texture->getHandle(), name); // In case of RE-2274 repro case
      recordTextureFreed(texture->getMipLevelRange(), texture->getArrayLayers(), texture->getBaseExtent(),
        !texture->isAliased() ? texture->getMemory().size() : 0, texture->getFormat(), name);
    });
    G_ASSERT(!bindless_manager.hasImageReference(texture));
  }
  deleteImageObjects(textures);
}

D3D12_RESOURCE_DESC AliasHeapProvider::as_desc(const BasicTextureResourceDescription &desc)
{
  auto format = FormatStore::fromCreateFlags(desc.cFlags);

  D3D12_RESOURCE_DESC result{};
  result.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  result.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  result.MipLevels = desc.mipLevels;
  result.Format = format.asDxGiTextureCreateFormat();
  result.SampleDesc.Count = get_sample_count(desc.cFlags);
  result.SampleDesc.Quality = 0;
  result.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  if (TEXCF_RTARGET & desc.cFlags)
  {
    if (format.isColor())
    {
      result.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    else
    {
      result.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
  }
  if (TEXCF_UNORDERED & desc.cFlags)
  {
    result.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }
  return result;
}

D3D12_RESOURCE_DESC AliasHeapProvider::as_desc(const TextureResourceDescription &desc)
{
  auto result = as_desc(static_cast<const BasicTextureResourceDescription &>(desc));
  result.Width = desc.width;
  result.Height = desc.height;
  result.DepthOrArraySize = 1;
  result.Alignment = calculate_texture_alignment(result.Width, result.Height, 1, result.SampleDesc.Count, result.Layout, result.Flags,
    FormatStore::fromCreateFlags(desc.cFlags));
  return result;
}

D3D12_RESOURCE_DESC AliasHeapProvider::as_desc(const VolTextureResourceDescription &desc)
{
  auto result = as_desc(static_cast<const TextureResourceDescription &>(desc));
  result.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
  result.DepthOrArraySize = desc.depth;
  result.Alignment = calculate_texture_alignment(result.Width, result.Height, desc.depth, result.SampleDesc.Count, result.Layout,
    result.Flags, FormatStore::fromCreateFlags(desc.cFlags));
  return result;
}

D3D12_RESOURCE_DESC AliasHeapProvider::as_desc(const ArrayTextureResourceDescription &desc)
{
  auto result = as_desc(static_cast<const TextureResourceDescription &>(desc));
  result.DepthOrArraySize = desc.arrayLayers;
  result.Alignment = calculate_texture_alignment(result.Width, result.Height, 1, result.SampleDesc.Count, result.Layout, result.Flags,
    FormatStore::fromCreateFlags(desc.cFlags));
  return result;
}

D3D12_RESOURCE_DESC AliasHeapProvider::as_desc(const CubeTextureResourceDescription &desc)
{
  auto result = as_desc(static_cast<const BasicTextureResourceDescription &>(desc));
  result.Width = desc.extent;
  result.Height = desc.extent;
  result.DepthOrArraySize = 6;
  result.Alignment = calculate_texture_alignment(result.Width, result.Height, 1, result.SampleDesc.Count, result.Layout, result.Flags,
    FormatStore::fromCreateFlags(desc.cFlags));
  return result;
}

D3D12_RESOURCE_DESC
AliasHeapProvider::as_desc(const ArrayCubeTextureResourceDescription &desc)
{
  auto result = as_desc(static_cast<const CubeTextureResourceDescription &>(desc));
  result.DepthOrArraySize = desc.cubes * 6;
  result.Alignment = calculate_texture_alignment(result.Width, result.Height, 1, result.SampleDesc.Count, result.Layout, result.Flags,
    FormatStore::fromCreateFlags(desc.cFlags));
  return result;
}

D3D12_RESOURCE_DESC AliasHeapProvider::as_desc(const BufferResourceDescription &desc)
{
  D3D12_RESOURCE_DESC result{};
  result.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  result.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  result.Width = desc.elementSizeInBytes * desc.elementCount;
  result.Height = 1;
  result.DepthOrArraySize = 1;
  result.MipLevels = 1;
  result.Format = DXGI_FORMAT_UNKNOWN;
  result.SampleDesc.Count = 1;
  result.SampleDesc.Quality = 0;
  result.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  result.Flags = D3D12_RESOURCE_FLAG_NONE;
  if ((SBCF_BIND_UNORDERED | SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE) & desc.cFlags)
  {
    result.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }
  return result;
}

D3D12_RESOURCE_DESC AliasHeapProvider::as_desc(const ResourceDescription &desc)
{
  switch (desc.resType)
  {
    case RES3D_TEX: return as_desc(desc.asTexRes);
    case RES3D_CUBETEX: return as_desc(desc.asCubeTexRes);
    case RES3D_VOLTEX: return as_desc(desc.asVolTexRes);
    case RES3D_ARRTEX: return as_desc(desc.asArrayTexRes);
    case RES3D_CUBEARRTEX: return as_desc(desc.asArrayCubeTexRes);
    case RES3D_SBUF: return as_desc(desc.asBufferRes);
  }

  return {};
}

DeviceMemoryClass AliasHeapProvider::get_memory_class(const ResourceDescription &desc)
{
  if (RES3D_SBUF == desc.resType)
  {
#if DX12_USE_ESRAM
    if ((desc.asBasicRes.cFlags & SBCF_MISC_ESRAM_ONLY) && hasESRam())
    {
      return DeviceMemoryClass::ESRAM_RESOURCE;
    }
#endif
    return GenericBufferInterface::get_memory_class(desc.asBasicRes.cFlags);
  }
  else
  {
#if DX12_USE_ESRAM
    if ((desc.asBasicRes.cFlags & TEXCF_ESRAM_ONLY) && hasESRam())
    {
      return DeviceMemoryClass::ESRAM_RESOURCE;
    }
#endif
    return BaseTex::get_memory_class(
      BaseTex::update_flags_for_linear_layout(desc.asBasicRes.cFlags, FormatStore::fromCreateFlags(desc.asBasicRes.cFlags)));
  }
}

uint32_t AliasHeapProvider::adoptMemoryAsAliasHeap(ResourceMemory memory)
{
  AliasHeap newAliasHeap{};
  newAliasHeap.memory = memory;
  newAliasHeap.flags.set(AliasHeap::Flag::AUTO_FREE);
  uint32_t index = 0;
  {
    auto aliasHeapsAccess = aliasHeaps.access();
    for (; index < aliasHeapsAccess->size(); ++index)
    {
      if ((*aliasHeapsAccess)[index])
      {
        continue;
      }
      (*aliasHeapsAccess)[index] = eastl::move(newAliasHeap);
      updateMemoryRangeUse(memory, AliasHeapReference{index});
      return index;
    }
    aliasHeapsAccess->push_back(newAliasHeap);
  }
  updateMemoryRangeUse(memory, AliasHeapReference{index});
  return index;
}

::ResourceHeap *AliasHeapProvider::newUserHeap(DXGIAdapter *adapter, Device &device, ::ResourceHeapGroup *group, size_t size,
  ResourceHeapCreateFlags flags)
{
  using UserResourceHeapType = ::ResourceHeap;
  AllocationFlags allocFlags{};
  if (RHCF_REQUIRES_DEDICATED_HEAP & flags)
  {
    allocFlags.set(AllocationFlag::DEDICATED_HEAP);
  }

  AliasHeap newPHeap{};

  auto properties = getHeapGroupProperties(group);

  // TODO: alignment has to be a part of the heap as well.
  D3D12_RESOURCE_ALLOCATION_INFO allocInfo{size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT};

  newPHeap.memory = allocate(adapter, device.getDevice(), properties, allocInfo, allocFlags);
  if (!newPHeap.memory)
  {
    device.processEmergencyDefragmentation(properties.raw, true, false, false);
    newPHeap.memory = allocate(adapter, device.getDevice(), properties, allocInfo, allocFlags);
  }
  const ResourceHeapProperties allocatedProperties{newPHeap.memory.getHeapID().group};
  if (!newPHeap.memory)
  {
    return nullptr;
  }
#ifdef _TARGET_XBOX
  HRESULT hr = device.getDevice()->RegisterPagePoolX(reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(newPHeap.memory.asPointer()),
    (newPHeap.memory.size() + TEXTURE_TILE_SIZE - 1) / TEXTURE_TILE_SIZE, &newPHeap.heapRegHandle);
  G_UNUSED(hr);
  G_ASSERT(SUCCEEDED(hr));
#endif
  recordNewUserResourceHeap(size, !allocatedProperties.isCPUVisible());
  uintptr_t index = 0;

  {
    auto aliasHeapsAccess = aliasHeaps.access();
    for (; index < aliasHeapsAccess->size(); ++index)
    {
      if ((*aliasHeapsAccess)[index])
      {
        continue;
      }
      updateMemoryRangeUse(newPHeap.memory, AliasHeapReference{uint32_t(index)});
      (*aliasHeapsAccess)[index] = eastl::move(newPHeap);
      // have to start with 1 as null -> nullptr which is invalid
      return reinterpret_cast<UserResourceHeapType *>(index + 1);
    }

    aliasHeapsAccess->push_back(eastl::move(newPHeap));
  }

  updateMemoryRangeUse(newPHeap.memory, AliasHeapReference{uint32_t(index)});
  // have to start with 1 as null -> nullptr which is invalid
  return reinterpret_cast<UserResourceHeapType *>(index + 1);
}

// assumes mutex is locked
void AliasHeapProvider::freeUserHeap(ID3D12Device *device, ::ResourceHeap *ptr)
{
  // heap ptr / index starts with 1, so adjust to start from 0
  auto index = reinterpret_cast<uintptr_t>(ptr) - 1;

  auto aliasHeapsAccess = aliasHeaps.access();
  G_ASSERTF(index < aliasHeapsAccess->size(), "DX12: Tried to free non existing resource heap %p", ptr);
  if (index >= aliasHeapsAccess->size())
  {
    D3D_ERROR("DX12: Tried to free non existing resource heap %p", ptr);
    return;
  }
  auto &heap = (*aliasHeapsAccess)[index];
  if (!heap)
  {
    D3D_ERROR("DX12: Tried to free already freed resource heap %p", ptr);
    return;
  }

  HEAP_LOG("DX12: Free user heap %p of size %.2f %s", heap.memory.getAddress(), ByteUnits{heap.memory.size()}.units(),
    ByteUnits{heap.memory.size()}.name());

  ResourceHeapProperties properties;
  properties.raw = heap.memory.getHeapID().group;
  recordDeletedUserResouceHeap(heap.memory.size(), !properties.isCPUVisible());

  G_ASSERTF(heap.images.empty() && heap.buffers.empty(), "DX12: Resources of a resource heap should be destroyed before the heap is "
                                                         "destroyed");
#ifdef _TARGET_XBOX
  device->UnregisterPagePoolX(heap.heapRegHandle);
#endif
  G_UNUSED(device);

  free(heap.memory, false);
  heap.reset();
}

ResourceAllocationProperties AliasHeapProvider::getResourceAllocationProperties(ID3D12Device *device, const ResourceDescription &desc)
{
  using ResourceHeapGroupType = ::ResourceHeapGroup;

  auto dxDesc = as_desc(desc);
  auto allocInfo = get_resource_allocation_info(device, dxDesc);

  ResourceAllocationProperties props{};
  if (is_valid_allocation_info(allocInfo))
  {
    G_STATIC_ASSERT(sizeof(ResourceAllocationProperties::heapGroup) >= sizeof(ResourceHeapProperties::raw));
    props.sizeInBytes = static_cast<uint32_t>(allocInfo.SizeInBytes);
    props.offsetAlignment = static_cast<uint32_t>(allocInfo.Alignment);
    props.heapGroup = reinterpret_cast<ResourceHeapGroupType *>(
      static_cast<uintptr_t>(getProperties(dxDesc.Flags, get_memory_class(desc), allocInfo.Alignment).raw));
  }
  else
  {
    report_resource_alloc_info_error(dxDesc);
  }

  return props;
}

ImageCreateResult AliasHeapProvider::placeTextureInHeap(DXGIAdapter *adapter, ID3D12Device *device, ::ResourceHeap *heap,
  const ResourceDescription &desc, size_t offset, const ResourceAllocationProperties &alloc_info, const char *name)
{
  ImageCreateResult result{};
  auto dxDesc = as_desc(desc);
  auto fmt = FormatStore::fromCreateFlags(desc.asBasicRes.cFlags);
  switch (desc.asBasicTexRes.activation)
  {
    case ResourceActivationAction::REWRITE_AS_COPY_DESTINATION: result.state = D3D12_RESOURCE_STATE_COPY_DEST; break;
    case ResourceActivationAction::REWRITE_AS_UAV:
    case ResourceActivationAction::CLEAR_F_AS_UAV:
    case ResourceActivationAction::CLEAR_I_AS_UAV:
    case ResourceActivationAction::DISCARD_AS_UAV:
      G_ASSERT(TEXCF_UNORDERED & desc.asBasicRes.cFlags);
      result.state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
      break;
    case ResourceActivationAction::REWRITE_AS_RTV_DSV:
    case ResourceActivationAction::CLEAR_AS_RTV_DSV:
    case ResourceActivationAction::DISCARD_AS_RTV_DSV:
      G_ASSERT(TEXCF_RTARGET & desc.asBasicRes.cFlags);
      if (fmt.isColor())
      {
        result.state = D3D12_RESOURCE_STATE_RENDER_TARGET;
      }
      else
      {
        result.state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
      }
      break;
  }

  // heap ptr / index starts with 1, so adjust to start from 0
  auto index = reinterpret_cast<uintptr_t>(heap) - 1;

  HRESULT errorCode = S_OK;
  auto oomCheckOnExit = checkForOOMOnExit(
    adapter, [&errorCode]() { return !is_oom_error_code(errorCode); },
    OomReportData{"placeTextureInHeap", name, alloc_info.sizeInBytes});

  auto aliasHeapsAccess = aliasHeaps.access();
  if (index >= aliasHeapsAccess->size())
  {
    return result;
  }
  auto &heapRef = (*aliasHeapsAccess)[index];
  auto memory = heapRef.memory.aliasSubRange(index, offset, alloc_info.sizeInBytes);

  D3D12_CLEAR_VALUE *clearValuePtr = nullptr;
  D3D12_CLEAR_VALUE clearValue{};
  if ((ResourceActivationAction::CLEAR_AS_RTV_DSV == desc.asBasicTexRes.activation) ||
      (ResourceActivationAction::CLEAR_F_AS_UAV == desc.asBasicTexRes.activation) ||
      (ResourceActivationAction::CLEAR_I_AS_UAV == desc.asBasicTexRes.activation))
  {
    // TODO: may be wrong for UAV stuff
    clearValuePtr = &clearValue;
    clearValue.Format = dxDesc.Format;
    if (fmt.isColor())
    {
      clearValue.Color[0] = desc.asBasicTexRes.clearValue.asFloat[0];
      clearValue.Color[1] = desc.asBasicTexRes.clearValue.asFloat[1];
      clearValue.Color[2] = desc.asBasicTexRes.clearValue.asFloat[2];
      clearValue.Color[3] = desc.asBasicTexRes.clearValue.asFloat[3];
    }
    else
    {
      clearValue.DepthStencil.Depth = desc.asBasicTexRes.clearValue.asFloat[0];
      clearValue.DepthStencil.Stencil = desc.asBasicTexRes.clearValue.asUint[1];
    }
  }

  ComPtr<ID3D12Resource> texture;
  errorCode =
#if _TARGET_XBOX
    DX12_CHECK_RESULT_NO_OOM_CHECK(
      xbox_create_placed_resource(device, memory.getAddress(), dxDesc, result.state, clearValuePtr, texture));
#else
    DX12_CHECK_RESULT_NO_OOM_CHECK(
      device->CreatePlacedResource(memory.getHeap(), memory.getOffset(), &dxDesc, result.state, clearValuePtr, COM_ARGS(&texture)));
#endif

  if (!texture)
  {
    return result;
  }

  Extent3D ext;
  ext.width = dxDesc.Width;
  ext.height = dxDesc.Height;
  auto mipMapCount = MipMapCount::make(dxDesc.MipLevels);
  ArrayLayerCount arrayLayerCount;
  if (D3D12_RESOURCE_DIMENSION_TEXTURE3D != dxDesc.Dimension)
  {
    ext.depth = 1;
    arrayLayerCount = ArrayLayerCount::make(dxDesc.DepthOrArraySize);
  }
  else
  {
    ext.depth = dxDesc.DepthOrArraySize;
    arrayLayerCount = ArrayLayerCount::make(1);
  }

  auto subResCount = mipMapCount * arrayLayerCount * fmt.getPlanes();

  auto subResIdBase = allocateGlobalResourceIdRange(subResCount);
  result.image = newImageObject(memory, eastl::move(texture), dxDesc.Dimension, dxDesc.Layout, fmt, ext, mipMapCount, arrayLayerCount,
    subResIdBase, get_log2i_of_pow2(dxDesc.SampleDesc.Count));

  recordTexturePlacedInUserResourceHeap(result.image->getMipLevelRange(), result.image->getArrayLayers(),
    result.image->getBaseExtent(), result.image->getMemory().size(), result.image->getFormat(), name);
  heapRef.images.push_back(result.image);

  result.image->setGPUChangeable(
    0 != (dxDesc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL |
                           D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)));

  return result;
}

#if _TARGET_PC_WIN
ResourceHeapGroupProperties AliasHeapProvider::getResourceHeapGroupProperties(::ResourceHeapGroup *heap_group)
{
  ResourceHeapGroupProperties result;
  result.flags = 0;

  if (isUMASystem())
  {
    result.isCPUVisible = true;
    result.isGPULocal = true;
    result.maxHeapSize = max(getHostLocalPhysicalLimit(), getDeviceLocalPhysicalLimit());
  }
  else
  {
    auto properties = getHeapGroupProperties(heap_group);
    if (properties.isL0Pool)
    {
      result.isCPUVisible = true;
      result.isGPULocal = false;
      result.maxHeapSize = getHostLocalPhysicalLimit();
    }
    else
    {
      result.isCPUVisible = false;
      result.isGPULocal = true;
      result.maxHeapSize = getDeviceLocalPhysicalLimit();
    }
  }
  // On PC there is currently no HW where we could access and control usage of on chip memory
  result.isOnChip = false;

  result.maxResourceSize = result.maxHeapSize;
  // Suggested optimal size on the DX discord by some MS representatives. Windows may not be able to
  // allocate larger heaps on the device and silently falls back to alternative memory sources.
  // Suggested sizes where 64 or 32 MiBytes, currently we will use fixed 64 MiBytes for now, but may be
  // later we use different values for different max available memory.
  constexpr uint64_t optimal_max_heap_size = 64 * 1024 * 1024;
  result.optimalMaxHeapSize = min(result.maxHeapSize, optimal_max_heap_size);
  return result;
}
#else
ResourceHeapGroupProperties AliasHeapProvider::getResourceHeapGroupProperties(::ResourceHeapGroup *heap_group)
{
  auto properties = getHeapGroupProperties(heap_group);

  ResourceHeapGroupProperties result;
  // no optimal size
  result.optimalMaxHeapSize = 0;
  result.flags = 0;

  result.isCPUVisible = true;
  result.isGPULocal = true;

#if DX12_USE_ESRAM
  if (properties.isESRAM)
  {
    result.isOnChip = true;
    result.maxResourceSize = result.maxHeapSize = getFreeEsram();
  }
  else
#endif
  {
    G_UNUSED(properties);
    result.isOnChip = false;
    size_t gameLimit = 0, gameUsed = 0;
    xbox_get_memory_status(gameUsed, gameLimit);
    result.maxResourceSize = result.maxHeapSize = gameLimit;
    // Workaround for OOM on xbox that is probably caused by an attempt to allocate too big heap when memory is fragmented.
    const uint64_t optimal_max_heap_size =
      ::dgs_get_settings()->getBlockByNameEx("dx12")->getInt("optimal_max_heap_size", 64 * 1024 * 1024);
    result.optimalMaxHeapSize = min(result.maxHeapSize, optimal_max_heap_size);
  }

  return result;
}
#endif

ResourceMemory AliasHeapProvider::getUserHeapMemory(::ResourceHeap *heap)
{
  ResourceMemory result{};
  auto index = reinterpret_cast<uintptr_t>(heap) - 1;

  {
    auto aliasHeapsAccess = aliasHeaps.access();
    if (index < aliasHeapsAccess->size())
    {
      result = (*aliasHeapsAccess)[index].memory;
    }
  }
  return result;
}

bool AliasHeapProvider::detachTextures(eastl::span<Image *> textures)
{
  bool needsAutoFreeHandling = false;
  auto aliasHeapsAccess = aliasHeaps.access();
  for (auto texture : textures)
  {
    auto heapId = texture->getMemory().getHeapID();
    if (heapId.isAlias)
    {
      G_ASSERT(heapId.index < aliasHeapsAccess->size());
      if (heapId.index < aliasHeapsAccess->size())
      {
        auto &heap = (*aliasHeapsAccess)[heapId.index];
        auto ref = eastl::find(begin(heap.images), end(heap.images), texture);
        G_ASSERT(ref != end(heap.images));
        if (ref != end(heap.images))
        {
          heap.images.erase(ref);
          needsAutoFreeHandling = needsAutoFreeHandling || heap.shouldBeFreed();
        }
      }
    }
  }
  return needsAutoFreeHandling;
}

void AliasHeapProvider::processAutoFree()
{
  auto aliasHeapsAccess = aliasHeaps.access();
  for (auto &heap : *aliasHeapsAccess)
  {
    if (!heap)
    {
      continue;
    }
    if (!heap.shouldBeFreed())
    {
      continue;
    }
    ResourceHeapProperties properties;
    properties.raw = heap.memory.getHeapID().group;
    recordDeletedUserResouceHeap(heap.memory.size(), !properties.isCPUVisible());
    free(heap.memory, false);
    heap.reset();
  }
}

BufferState AliasHeapProvider::placeBufferInHeap(DXGIAdapter *adapter, ID3D12Device *device, ::ResourceHeap *heap,
  const ResourceDescription &desc, size_t offset, const ResourceAllocationProperties &alloc_info, const char *name)
{
  BufferState result;
  // heap ptr / index starts with 1, so adjust to start from 0
  auto index = reinterpret_cast<uintptr_t>(heap) - 1;

  HRESULT errorCode = S_OK;
  auto oomCheckOnExit = checkForOOMOnExit(
    adapter, [&errorCode]() { return !is_oom_error_code(errorCode); },
    OomReportData{"placeBufferInHeap", name, alloc_info.sizeInBytes});

  auto aliasHeapsAccess = aliasHeaps.access();
  if (index >= aliasHeapsAccess->size())
  {
    D3D_ERROR("DX12: placeBufferInHeap failed (heap does not exists)");
    return result;
  }
  auto &heapRef = (*aliasHeapsAccess)[index];

  D3D12_RESOURCE_DESC dxDesc = as_desc(desc);

  D3D12_RESOURCE_STATES state =
    ((SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE | SBCF_BIND_UNORDERED) & desc.asBufferRes.cFlags)
      ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
      : D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE;

  BufferHeap::Heap newHeap;
  auto heapProperties = getHeapGroupProperties(alloc_info.heapGroup);
  auto memory = heapRef.memory.aliasSubRange(index, offset, alloc_info.sizeInBytes);

  errorCode = newHeap.create(device, dxDesc, memory, state, heapProperties.isCPUVisible());

  if (DX12_CHECK_FAIL(errorCode))
  {
    return result;
  }

  // Leave free ranges empty so that sub allocation never tries to use our buffers
  newHeap.flags = dxDesc.Flags;

  BufferGlobalId resultID;
  {
    auto bufferHeapStateAccess = bufferHeapState.access();
    resultID = bufferHeapStateAccess->adoptBufferHeap(eastl::move(newHeap));
    heapRef.buffers.push_back(resultID);

    auto &selectedHeap = bufferHeapStateAccess->bufferHeaps[resultID.index()];
    result.buffer = selectedHeap.buffer.Get();
    result.size = dxDesc.Width;
    result.discardCount = 1;
    result.resourceId = resultID;
#if _TARGET_PC_WIN
    result.cpuPointer = selectedHeap.getCPUPointer();
#endif
    result.gpuPointer = selectedHeap.getGPUPointer();
  }

  recordBufferPlacedInUserResouceHeap(result.size, !heapProperties.isCPUVisible(), name);

  return result;
}

bool AliasHeapProvider::detachBuffer(const BufferState &buf)
{
  HeapID heapID;
  {
    auto bufferHeapStateAccess = bufferHeapState.access();
    auto bufferIndex = buf.resourceId.index();
    if (bufferIndex >= bufferHeapStateAccess->bufferHeaps.size())
    {
      return false;
    }
    auto &buffer = bufferHeapStateAccess->bufferHeaps[buf.resourceId.index()];
    heapID = buffer.bufferMemory.getHeapID();
  }
  if (heapID.isAlias)
  {
    auto aliasHeapsAccess = aliasHeaps.access();
    G_ASSERT(heapID.index < aliasHeapsAccess->size());
    if (heapID.index < aliasHeapsAccess->size())
    {
      auto &heap = (*aliasHeapsAccess)[heapID.index];
      auto ref = eastl::find(begin(heap.buffers), end(heap.buffers), buf.resourceId);
      G_ASSERT(ref != end(heap.buffers));
      if (ref != end(heap.buffers))
      {
        heap.buffers.erase(ref);
        return heap.shouldBeFreed();
      }
    }
  }
  return false;
}

ImageCreateResult AliasHeapProvider::aliasTexture(DXGIAdapter *adapter, ID3D12Device *device, const ImageInfo &ii, Image *base,
  const char *name)
{
  ImageCreateResult result{};
  auto desc = ii.asDesc();

  auto baseMemory = base->getMemory();
#if _TARGET_PC_WIN
  if (!baseMemory)
  {
    base->getDebugName([name](auto &base_name) {
      logwarn("DX12: Can not alias textures '%s' with '%s'. No heap associated with '%s'. Possibly a committed resource.", base_name,
        name, base_name);
    });
    return result;
  }
#endif

  auto allocInfo = get_resource_allocation_info(device, desc);
  if (!is_valid_allocation_info(allocInfo))
  {
    report_resource_alloc_info_error(desc);
    return result;
  }

  auto baseHeapID = baseMemory.getHeapID();
  ResourceHeapProperties baseMemoryProperties;
  baseMemoryProperties.raw = baseHeapID.group;

  auto memoryProperties = getProperties(desc.Flags, ii.memoryClass, allocInfo.Alignment);
  if (baseMemoryProperties != memoryProperties)
  {
    base->getDebugName([name, baseMemoryProperties, memoryProperties](auto &base_name) {
      logwarn("DX12: Can not alias textures '%s' with '%s' as the memory properties %u and %u are "
              "incompatible",
        base_name, name, baseMemoryProperties.raw, memoryProperties.raw);
    });
    return result;
  }


  bool freshAlias = false;
  // if the base is not already handled by the alias heap system, adopt its memory and transform
  // the resource into a alias resource
  if (0 == baseHeapID.isAlias)
  {
    auto index = adoptMemoryAsAliasHeap(baseMemory);
    baseMemory = baseMemory.aliasSubRange(index, 0, baseMemory.size());
    baseHeapID = baseMemory.getHeapID();
    base->replaceMemory(baseMemory);
    freshAlias = true;
  }

  HRESULT errorCode = S_OK;
  auto oomCheckOnExit = checkForOOMOnExit(
    adapter, [&errorCode]() { return !is_oom_error_code(errorCode); }, OomReportData{"aliasTexture", name, allocInfo.SizeInBytes});
  {
    auto aliasHeapsAccess = aliasHeaps.access();
    auto &heap = (*aliasHeapsAccess)[baseHeapID.index];
    if (freshAlias)
    {
      heap.images.push_back(base);
    }

    bool isAdoptedHeap = heap.flags.test(AliasHeap::Flag::AUTO_FREE);
    if (isAdoptedHeap && allocInfo.SizeInBytes > heap.memory.size())
    {
      logwarn("DX12: Tried to create aliasing texture with insufficient memory from base, base "
              "can provide %u bytes but alias needs %u bytes",
        heap.memory.size(), allocInfo.SizeInBytes);
      return result;
    }

    uint32_t offsetInHeap = isAdoptedHeap ? 0 : heap.memory.calculateOffset(baseMemory);
    if (!isAdoptedHeap && offsetInHeap + allocInfo.SizeInBytes > heap.memory.size())
    {
      logwarn("DX12: Tried to create aliasing texture on insufficient memory region of heap, "
              "heap can provide %u bytes but alias needs %u bytes at offset %u",
        heap.memory.size(), allocInfo.SizeInBytes, offsetInHeap);
      return result;
    }

    auto memory = heap.memory.aliasSubRange(baseHeapID.index, offsetInHeap, allocInfo.SizeInBytes);

    result.state = getInitialTextureResourceState(ii.usage);

    ComPtr<ID3D12Resource> texture;
    errorCode =
#if _TARGET_XBOX
      DX12_CHECK_RESULT_NO_OOM_CHECK(xbox_create_placed_resource(device, memory.getAddress(), desc, result.state, nullptr, texture));
#else
      DX12_CHECK_RESULT_NO_OOM_CHECK(
        device->CreatePlacedResource(memory.getHeap(), memory.getOffset(), &desc, result.state, nullptr, COM_ARGS(&texture)));
#endif

    if (!texture)
    {
      return result;
    }

    auto subResIdBase = allocateGlobalResourceIdRange(ii.getSubResourceCount());
    result.image = newImageObject(memory, eastl::move(texture), ii.type, ii.memoryLayout, ii.format, ii.size, ii.mips, ii.arrays,
      subResIdBase, get_log2i_of_pow2(ii.sampleDesc.Count));

    heap.images.push_back(result.image);
  }

  recordTextureAliased(result.image->getMipLevelRange(), result.image->getArrayLayers(), result.image->getBaseExtent(),
    result.image->getFormat(), name);
  result.image->setGPUChangeable(
    0 != (ii.usage & (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL |
                       D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)));
  return result;
}

const MetricsProviderBase::MetricNameTableEntry MetricsProviderBase::metric_name_table[static_cast<uint32_t>(Metric::COUNT)] = //
  {
    {MetricsProviderBase::Metric::EVENT_LOG, "Event Log"},
    {MetricsProviderBase::Metric::METRIC_LOG, "Metric Log"},
    {MetricsProviderBase::Metric::MEMORY_USE, "Memory Use"},
    {MetricsProviderBase::Metric::HEAPS, "Heaps"},
    {MetricsProviderBase::Metric::MEMORY, "Memory"},
    {MetricsProviderBase::Metric::BUFFERS, "Buffers"},
    {MetricsProviderBase::Metric::TEXTURES, "Textures"},
    {MetricsProviderBase::Metric::CONST_RING, "Const Ring"},
    {MetricsProviderBase::Metric::TEMP_RING, "Temp Ring"},
    {MetricsProviderBase::Metric::TEMP_MEMORY, "Temp Memory"},
    {MetricsProviderBase::Metric::PERSISTENT_UPLOAD, "Persistent Upload"},
    {MetricsProviderBase::Metric::PERSISTENT_READ_BACK, "Persistent Read Back"},
    {MetricsProviderBase::Metric::PERSISTENT_BIDIRECTIONAL, "Persistent Bidirectional"},
    {MetricsProviderBase::Metric::SCRATCH_BUFFER, "Scratch Buffer"},
    {MetricsProviderBase::Metric::RAYTRACING, "Raytracing"},
};

const char *MetricsProviderBase::graph_name_table[static_cast<uint32_t>(Graph::COUNT)] = //
  {"System", "Const Ring", "Upload Ring", "Temp Memory", "Upload Memory", "Read Back Memory", "Bidirectional Memory", "Buffers",
    "Heaps", "Textures", "User Heaps", "Scratch Buffer"};

const char *MetricsProviderBase::graph_mode_name_table[4] = //
  {"Block Scrolling", "Contiguous Scrolling", "Full Range", "Disabled"};

const MetricsProviderBase::Metric
  MetricsProviderBase::event_to_metric_type_table[static_cast<uint32_t>(MetricsState::ActionInfo::Type::COUNT)] = //
  {MetricsProviderBase::Metric::HEAPS, MetricsProviderBase::Metric::HEAPS, MetricsProviderBase::Metric::MEMORY,
    MetricsProviderBase::Metric::MEMORY, MetricsProviderBase::Metric::BUFFERS, MetricsProviderBase::Metric::BUFFERS,
    MetricsProviderBase::Metric::TEXTURES, MetricsProviderBase::Metric::TEXTURES, MetricsProviderBase::Metric::TEXTURES,
    MetricsProviderBase::Metric::TEXTURES, MetricsProviderBase::Metric::TEXTURES, MetricsProviderBase::Metric::CONST_RING,
    MetricsProviderBase::Metric::CONST_RING, MetricsProviderBase::Metric::TEMP_RING, MetricsProviderBase::Metric::TEMP_RING,
    MetricsProviderBase::Metric::TEMP_MEMORY, MetricsProviderBase::Metric::TEMP_MEMORY, MetricsProviderBase::Metric::TEMP_MEMORY,
    MetricsProviderBase::Metric::PERSISTENT_UPLOAD, MetricsProviderBase::Metric::PERSISTENT_UPLOAD,
    MetricsProviderBase::Metric::PERSISTENT_UPLOAD, MetricsProviderBase::Metric::PERSISTENT_UPLOAD,
    MetricsProviderBase::Metric::PERSISTENT_READ_BACK, MetricsProviderBase::Metric::PERSISTENT_READ_BACK,
    MetricsProviderBase::Metric::PERSISTENT_READ_BACK, MetricsProviderBase::Metric::PERSISTENT_READ_BACK,
    MetricsProviderBase::Metric::PERSISTENT_BIDIRECTIONAL, MetricsProviderBase::Metric::PERSISTENT_BIDIRECTIONAL,
    MetricsProviderBase::Metric::PERSISTENT_BIDIRECTIONAL, MetricsProviderBase::Metric::PERSISTENT_BIDIRECTIONAL,
    // may introduce new metric for user heaps?
    MetricsProviderBase::Metric::HEAPS, MetricsProviderBase::Metric::HEAPS, MetricsProviderBase::Metric::BUFFERS,
    MetricsProviderBase::Metric::TEXTURES, MetricsProviderBase::Metric::SCRATCH_BUFFER, MetricsProviderBase::Metric::SCRATCH_BUFFER,
    MetricsProviderBase::Metric::RAYTRACING, MetricsProviderBase::Metric::RAYTRACING, MetricsProviderBase::Metric::RAYTRACING,
    MetricsProviderBase::Metric::RAYTRACING, MetricsProviderBase::Metric::RAYTRACING, MetricsProviderBase::Metric::RAYTRACING};

const char *MetricsProviderBase::event_type_name_table[static_cast<uint32_t>(MetricsState::ActionInfo::Type::COUNT)] = //
  {
    "Heap allocated",
    "Heap freed",
    "Memory allocated",
    "Memory freed",
    "Buffer allocated",
    "Buffer freed",
    "Texture allocated",
    "Texture aliased",
    "Texture adopted",
    "Texture allocated (ESRam)",
    "Texture freed",
    "Constant ring buffer allocated",
    "Constant ring buffer freed",
    "Temporary ring buffer allocated",
    "Temporary ring buffer freed",
    "Temporary buffer allocated",
    "Temporary buffer freed",
    "Temporary buffer used",
    "Persistent upload buffer allocated",
    "Persistent upload buffer freed",
    "Persistent upload memory allocated",
    "Persistent upload memory freed",
    "Persistent read back buffer allocated",
    "Persistent read back buffer freed",
    "Persistent read back memory allocated",
    "Persistent read back memory freed",
    "Persistent bidirectional buffer allocated",
    "Persistent bidirectional buffer freed",
    "Persistent bidirectional memory allocated",
    "Persistent bidirectional memory freed",
    "User resource heap allocated",
    "User resource heap freed",
    "User placed buffer",
    "User placed texture",
    "Scratch buffer allocate",
    "Scratch buffer free",
    "Raytrace bottom acceleration structure allocate",
    "Raytrace bottom acceleration structure free",
    "Raytrace top acceleration structure allocate",
    "Raytrace top acceleration structure free",
};

// IMGUI debug UI
#if DAGOR_DBGLEVEL > 0

#include <gui/dag_imgui.h>
#include <ioSys/dag_dataBlock.h>
#include <imgui.h>
#include <implot.h>
#include <psapi.h>

namespace
{
struct ScopedStyleColor
{
  template <typename T>
  ScopedStyleColor(ImGuiCol idx, T &&col)
  {
    ImGui::PushStyleColor(idx, eastl::forward<T>(col));
  }
  ~ScopedStyleColor() { ImGui::PopStyleColor(); }
};

struct ScopedWarningTextStyle : ScopedStyleColor
{
  ScopedWarningTextStyle() : ScopedStyleColor{ImGuiCol_Text, ImVec4(.75f, .75f, 0.f, 1.f)} {}
};

struct ScopedPlotStyleVar
{
  template <typename T>
  ScopedPlotStyleVar(ImGuiStyleVar var, T &&value)
  {
    ImPlot::PushStyleVar(var, eastl::forward<T>(value));
  }
  ~ScopedPlotStyleVar() { ImPlot::PopStyleVar(); }
};

struct ScopedTooltip
{
  ScopedTooltip()
  {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
  }
  ~ScopedTooltip()
  {
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
};

// needs to be paired with end_sub_section regardless of returned value
// returns true if the contents should be rendered
bool begin_sub_section(const char *id, const char *caption, int height)
{
  if (ImGui::BeginChild(id, ImVec2(0, height), ImGuiChildFlags_Border, ImGuiWindowFlags_MenuBar))
  {
    if (ImGui::BeginMenuBar())
    {
      ImGui::TextUnformatted(caption);
      ImGui::EndMenuBar();
    }
    return true;
  }
  return false;
}

void end_sub_section() { ImGui::EndChild(); }

void begin_selectable_row(const char *text)
{
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  ImGui::Selectable(text, false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
}

void draw_segment(void *base, ValueRange<uint64_t> range, uint64_t max, const char *text)
{
  ByteUnits size = range.size();
  char strBuf[MAX_OBJECT_NAME_LENGTH + 64];
  // triple # will use only the part after the # to generate the id, exactly what we want
  sprintf_s(strBuf, "%s###%p+%u", text, base, range.front());
  begin_selectable_row(strBuf);
  ImGui::TableSetColumnIndex(2);
  ImGui::Text("%016llX", range.front());
  ImGui::TableSetColumnIndex(3);
  ImGui::Text("%016llX", range.back() + 1);
  ImGui::TableSetColumnIndex(6);
  ImGui::Text("%.2f %s", size.units(), size.name());
  G_UNUSED(max);
}

void draw_segment(void *base, uint64_t from, uint64_t to, uint64_t max, const char *text)
{
  ByteUnits size = to - from;
  char strBuf[MAX_OBJECT_NAME_LENGTH + 64];
  // triple # will use only the part after the # to generate the id, exactly what we want
  sprintf_s(strBuf, "%s###%p+%u", text, base, from);
  begin_selectable_row(strBuf);
  ImGui::TableSetColumnIndex(2);
  ImGui::Text("%016llX", from);
  ImGui::TableSetColumnIndex(3);
  ImGui::Text("%016llX", to);
  ImGui::TableSetColumnIndex(6);
  ImGui::Text("%.2f %s", size.units(), size.name());
  G_UNUSED(max);
}


template <typename T, typename U, typename H>
uint32_t iterate_images_for_heap(T &images, H &heap, uint32_t offset, U action)
{
  uint32_t nextOffset = offset;
  // as we support aliasing we can not early exit
  images.iterateAllocated([&heap, offset, &nextOffset, &action](auto image) //
    {
      auto mem = image->getMemory();
      if (!heap.isPartOf(mem))
      {
        return;
      }
      if (offset != heap.calculateOffset(mem))
      {
        return;
      }
      action(image);
      nextOffset = max(nextOffset, offset + mem.size());
    });
  return nextOffset;
}


template <typename T, typename U, typename H>
uint32_t iterate_buffers_for_heap(T &buffers, H &heap, uint32_t offset, U action)
{
  uint32_t nextOffset = offset;
  // as we support aliasing we can not early exit
  for (auto &buffer : buffers)
  {
    if (!heap.isPartOf(buffer.bufferMemory))
    {
      continue;
    }
    if (offset != heap.calculateOffset(buffer.bufferMemory))
    {
      continue;
    }
    action(buffer);
    nextOffset = max(nextOffset, offset + buffer.bufferMemory.size());
  }
  return nextOffset;
}

void draw_segment_row(uint8_t *cpu_pointer, D3D12_GPU_VIRTUAL_ADDRESS gpu_pointer, uint64_t from, uint64_t to, const char *name)
{
  begin_selectable_row(name);
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("%p", cpu_pointer + from);
  ImGui::TableSetColumnIndex(2);
  ImGui::Text("%016I64X", gpu_pointer + from);
  ImGui::TableSetColumnIndex(3);
  ImGui::TextDisabled("--");
  ImGui::TableSetColumnIndex(4);
  auto size = to - from;
  auto sizeUnits = size_to_unit_table(size);
  ImGui::Text("%.2f %s", compute_unit_type_size(size, sizeUnits), get_unit_name(sizeUnits));
};

void make_table_header(std::initializer_list<const char *> names)
{
  for (auto &&name : names)
  {
    ImGui::TableSetupColumn(name);
  }
  ImGui::TableHeadersRow();
}

void show_wrong_driver_info()
{
  ScopedWarningTextStyle style;
  ImGui::TextUnformatted("DX12 Driver not used, no data available");
}

void begin_event_row(uint64_t frame, uint64_t ident, int time_index, const char *msg)
{
  char frameText[64];
  sprintf_s(frameText, "%I64u###Event%I64d", frame, ident);
  begin_selectable_row(frameText);
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("%3d.%02d", time_index / 1000, (time_index % 1000) / 10);
  ImGui::TableSetColumnIndex(2);
  ImGui::TextUnformatted(msg);
}

using PlotGetFunctionType = ImPlotPoint (*)(int, void *);

template <typename T>
void plot_shaded(const char *name, PlotGetFunctionType value_get, PlotGetFunctionType base_get, T &data)
{
  ImPlot::PlotShadedG(name, value_get, &data, base_get, &data, data.getViewSize());
}

template <typename T>
void plot_line(const char *name, PlotGetFunctionType value_get, T &data)
{
  ImPlot::PlotLineG(name, value_get, &data, data.getViewSize());
}

template <typename T>
struct PlotDataBase : T
{
  using T::T;

  static ImPlotPoint getAllocated(int idx, void *data) { return reinterpret_cast<PlotDataBase<T> *>(data)->getAllocatedPoint(idx); }
  static ImPlotPoint getUsed(int idx, void *data) { return reinterpret_cast<PlotDataBase<T> *>(data)->getUsedPoint(idx); }
  static ImPlotPoint getBase(int idx, void *data)
  {
    return {double(reinterpret_cast<PlotDataBase<T> *>(data)->getFrame(idx).frameIndex), double(0)};
  }
};

template <typename T1, typename T2>
void draw_ring_buffer_plot(const char *plot_segment_id, const char *plot_segment_label, const char *plot_label, T1 create_controls,
  T2 select_plot_data)
{
  const int child_height = 17 * ImGui::GetFrameHeightWithSpacing();
  if (begin_sub_section(plot_segment_id, plot_segment_label, child_height))
  {
    create_controls();

    if (ImPlot::BeginPlot(plot_label, nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend, ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit,
          ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
      auto data = select_plot_data();

      if (data.hasAnyData())
      {
        {
          ScopedPlotStyleVar shadedAlpha{ImPlotStyleVar_FillAlpha, 0.25f};
          plot_shaded("Active", data.getAllocated, data.getBase, data);
          plot_shaded("Usage", data.getUsed, data.getBase, data);
        }
        plot_line("Active", data.getAllocated, data);
        plot_line("Usage", data.getUsed, data);

        // when hovering the plot we show the frame and the total buffer size and usage at that
        // frame
        if (ImPlot::IsPlotHovered())
        {
          // those are actually the values in the graph where the mouse is pointing at
          auto pos = ImPlot::GetPlotMousePos();
          if (pos.x >= 0)
          {
            auto frameIdx = static_cast<uint64_t>(round(pos.x));
            if (data.selectFrame(frameIdx))
            {
              ScopedTooltip tooltip;
              ImGui::Text("Frame: %I64u", frameIdx);

              auto allocatedValue = data.getSelectedAllocate();
              auto allocatedValueUnits = size_to_unit_table(allocatedValue);
              ImPlot::ItemIcon(ImPlot::GetColormapColor(0));
              ImGui::SameLine();
              ImGui::Text("Active: %.2f %s", compute_unit_type_size(allocatedValue, allocatedValueUnits),
                get_unit_name(allocatedValueUnits));

              auto usedValue = data.getSelectedUsed();
              auto usedValueUnits = size_to_unit_table(usedValue);
              ImPlot::ItemIcon(ImPlot::GetColormapColor(1));
              ImGui::SameLine();
              ImGui::Text("Usage: %.2f %s", compute_unit_type_size(usedValue, usedValueUnits), get_unit_name(usedValueUnits));
            }
          }
        }
      }
      else
      {
        // When no data is available just display a empty plot
        ImPlot::PlotDummy("Active");
        ImPlot::PlotDummy("Usage");
        // Tell the user, there nothing to show in the plot
        ImPlot::PushStyleColor(ImPlotCol_InlayText, ImVec4(.75f, .75f, 0.f, 1.f));
        ImPlot::PlotText("No historical data available", 0.5, 0.5);
        ImPlot::PopStyleColor();
      }
      ImPlot::EndPlot();
    }
  }
  end_sub_section();
}

bool filter_object_name(const eastl::string &object_name, const char *object_filter)
{
  if (object_filter[0] == '\0')
  {
    return false;
  }
  if (object_name.empty())
  {
    return true;
  }
  return nullptr == strstr(object_name.c_str(), object_filter);
}

template <typename T>
void draw_persisten_buffer_table_rows(const T &buffer)
{
  auto sizeUnits = size_to_unit_table(buffer.bufferMemory.size());
  size_t freeMem = eastl::accumulate(begin(buffer.freeRanges), end(buffer.freeRanges), size_t(0),
    [](auto first, auto range) { return first + range.size(); });
  size_t usage = buffer.bufferMemory.size() - freeMem;
  auto usageUnits = size_to_unit_table(usage);
  char strBuf[32];
  sprintf_s(strBuf, "%p", buffer.buffer.Get());
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  bool open = ImGui::TreeNodeEx(strBuf, ImGuiTreeNodeFlags_SpanFullWidth);
  ImGui::TableSetColumnIndex(1);
  ImGui::Text("%p", buffer.getCPUPointer());
  ImGui::TableSetColumnIndex(2);
  ImGui::Text("%016I64X", buffer.getGPUPointer());
  ImGui::TableSetColumnIndex(3);
  ImGui::Text("%.2f %s", compute_unit_type_size(usage, usageUnits), get_unit_name(usageUnits));
  ImGui::TableSetColumnIndex(4);
  ImGui::Text("%.2f %s", compute_unit_type_size(buffer.bufferMemory.size(), sizeUnits), get_unit_name(sizeUnits));

  if (open)
  {
    if (buffer.freeRanges.empty())
    {
      draw_segment_row(buffer.getCPUPointer(), buffer.getGPUPointer(), 0, buffer.bufferMemory.size(), "Allocated");
    }
    else
    {
      size_t lastSegmentEnd = 0;
      for (auto &segment : buffer.freeRanges)
      {
        if (lastSegmentEnd < segment.front())
        {
          draw_segment_row(buffer.getCPUPointer(), buffer.getGPUPointer(), lastSegmentEnd, segment.front(), "Allocated");
          lastSegmentEnd = segment.front();
        }
        draw_segment_row(buffer.getCPUPointer(), buffer.getGPUPointer(), segment.front(), segment.back() + 1, "Free");
        lastSegmentEnd = segment.back() + 1;
      }
      if (lastSegmentEnd < buffer.bufferMemory.size())
      {
        draw_segment_row(buffer.getCPUPointer(), buffer.getGPUPointer(), lastSegmentEnd, buffer.bufferMemory.size(), "Allocated");
      }
    }
    ImGui::TreePop();
  }
}
} // namespace


void DebugViewBase::storeViewSettings()
{
  using eastl::begin;
  using eastl::end;

  auto blk = imgui_get_blk();
  if (!blk)
  {
    logdbg("DX12: ResourceMemoryHeap::storeViewSettings 'imgui_get_blk' returned null");
    return;
  }

  auto dx12Block = blk->addBlock("dx12");
  if (!dx12Block)
  {
    logdbg("DX12: ResourceMemoryHeap::storeViewSettings unable to write dx12 block");
    return;
  }

  auto memoryBlock = dx12Block->addBlock("memory");
  if (!memoryBlock)
  {
    logdbg("DX12: ResourceMemoryHeap::storeViewSettings unable to write memory block");
    return;
  }
  memoryBlock->setStr("event_object_name_filter", getEventObjectNameFilterBasePointer());

  for (auto &e : metric_name_table)
  {
    if ((Metric::EVENT_LOG == e.metric) || (Metric::METRIC_LOG == e.metric) || (Metric::MEMORY_USE == e.metric))
    {
      continue;
    }
    char strBuf[256];
    sprintf_s(strBuf, "show_metric_%s", e.name);
    memoryBlock->setBool(strBuf, isShownMetric(e.metric));
  }

  auto graphBlock = memoryBlock->addBlock("graph");
  if (graphBlock)
  {
    iterateGraphDisplayInfos([graphBlock](auto graph, const auto &state) //
      {
        char strBuf[256];
        sprintf_s(strBuf, "mode_%u", static_cast<uint32_t>(graph));
        graphBlock->setInt(strBuf, static_cast<int>(state.mode));

        sprintf_s(strBuf, "window_%u", static_cast<uint32_t>(graph));
        graphBlock->setInt64(strBuf, state.windowSize);
      });
  }

  imgui_save_blk();
}

void DebugViewBase::restoreViewSettings()
{
  using eastl::begin;
  using eastl::end;

  auto blk = imgui_get_blk();
  if (!blk)
  {
    logdbg("DX12: ResourceMemoryHeap::restoreViewSettings 'imgui_get_blk' returned null");
    return;
  }

  auto dx12Block = blk->getBlockByNameEx("dx12");
  if (!dx12Block)
  {
    logdbg("DX12: ResourceMemoryHeap::restoreViewSettings unable to read dx12 block");
    return;
  }

  auto memoryBlock = dx12Block->getBlockByNameEx("memory");
  if (!memoryBlock)
  {
    logdbg("DX12: ResourceMemoryHeap::restoreViewSettings unable to read memory block");
    return;
  }
  auto filter = memoryBlock->getStr("event_object_name_filter", "");
  if (filter)
  {
    if (strlen(filter) >= getEventObjectNameFilterMaxLength() - 1)
    {
      D3D_ERROR("DX12: Restore of dx12/memory/event-object-name-filter with malicious string");
    }
    else
    {
      strcpy(getEventObjectNameFilterBasePointer(), filter);
    }
  }

  for (auto &e : metric_name_table)
  {
    if ((Metric::EVENT_LOG == e.metric) || (Metric::METRIC_LOG == e.metric) || (Metric::MEMORY_USE == e.metric))
    {
      continue;
    }
    char strBuf[256];
    sprintf_s(strBuf, "show_metric_%s", e.name);
    // show heap events by default
    setShownMetric(e.metric, memoryBlock->getBool(strBuf, Metric::HEAPS == e.metric));
  }

  auto graphBlock = memoryBlock->getBlockByNameEx("graph");
  if (graphBlock)
  {
    iterateGraphDisplayInfos([graphBlock](auto graph, auto &state) //
      {
        char strBuf[256];
        sprintf_s(strBuf, "mode_%u", static_cast<uint32_t>(graph));
        auto rawMode = graphBlock->getInt(strBuf, static_cast<int>(GraphMode::BLOCK_SCROLLING));
        if (rawMode >= 0 && rawMode <= static_cast<int>(GraphMode::DISABLED))
        {
          state.mode = static_cast<GraphMode>(rawMode);
        }

        sprintf_s(strBuf, "window_%u", static_cast<uint32_t>(graph));
        state.windowSize = graphBlock->getInt64(strBuf, default_graph_window_size);
      });
  }

  // some things may set store flag, clear it or we write the config for no reason
  setStatusFlag(StatusFlag::STORE_VIEWS_TO_CONFIG, false);
}

#if DX12_SUPPORT_RESOURCE_MEMORY_METRICS
void MetricsVisualizer::drawPersistentBufferBasicMetricsTable(const char *id, const CounterWithSizeInfo &usage_counter,
  const CounterWithSizeInfo &buffer_counter, const PeakCounterWithSizeInfo &peak_memory_counter,
  const PeakCounterWithSizeInfo &peak_buffer_counter)
{
  if (ImGui::BeginTable(id, 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    make_table_header({"Metric", "Allocations", "Size"});

    const bool hasData = 0 != peak_memory_counter.count;

    begin_selectable_row("Last frame usage");
    if (hasData)
    {
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%I64u", usage_counter.count);
      ImGui::TableSetColumnIndex(2);
      auto allocationSizeUnit = size_to_unit_table(usage_counter.size);
      ImGui::Text("%.2f %s", compute_unit_type_size(usage_counter.size, allocationSizeUnit), get_unit_name(allocationSizeUnit));
    }

    begin_selectable_row("Last frame buffer");
    ImGui::TableNextColumn();
    if (hasData)
    {
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%I64u", buffer_counter.count);
      ImGui::TableSetColumnIndex(2);
      auto totalAllocationSizeUnit = size_to_unit_table(buffer_counter.size);
      ImGui::Text("%.2f %s", compute_unit_type_size(buffer_counter.size, totalAllocationSizeUnit),
        get_unit_name(totalAllocationSizeUnit));
    }

    char strBuf[64];
    sprintf_s(strBuf, "Peak usage at frame %I64u", peak_memory_counter.index);
    begin_selectable_row(strBuf);
    if (hasData)
    {
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%I64u", peak_memory_counter.count);
      ImGui::TableSetColumnIndex(2);
      auto metricsPeaksUnit = size_to_unit_table(peak_memory_counter.size);
      ImGui::Text("%.2f %s", compute_unit_type_size(peak_memory_counter.size, metricsPeaksUnit), get_unit_name(metricsPeaksUnit));
    }

    sprintf_s(strBuf, "Peak buffer at frame %I64u", peak_buffer_counter.index);
    begin_selectable_row(strBuf);
    if (hasData)
    {
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%I64u", peak_buffer_counter.count);
      ImGui::TableSetColumnIndex(2);
      auto metricsPeaksUnit = size_to_unit_table(peak_buffer_counter.size);
      ImGui::Text("%.2f %s", compute_unit_type_size(peak_buffer_counter.size, metricsPeaksUnit), get_unit_name(metricsPeaksUnit));
    }

    // when no data is available tell this in a tooltip
    if (!hasData)
    {
      if (ImGuiTableColumnFlags_IsHovered &
          (ImGui::TableGetColumnFlags(0) | ImGui::TableGetColumnFlags(1) | ImGui::TableGetColumnFlags(2)))
      {
        ScopedTooltip tooltip;
        ScopedWarningTextStyle warningStyle;
        ImGui::Bullet();
        ImGui::TextUnformatted("No memory usage data available");
      }
    }

    ImGui::EndTable();
  }
}

void MetricsVisualizer::drawRingMemoryBasicMetricsTable(const char *id, const CounterWithSizeInfo &current_counter,
  const CounterWithSizeInfo &summary_counter, const PeakCounterWithSizeInfo &peak_counter,
  const PeakCounterWithSizeInfo &peak_buffer_counter)
{

  if (ImGui::BeginTable(id, 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    make_table_header({"Metric", "Allocations", "Size"});

    const bool hasData = 0 != summary_counter.count;

    begin_selectable_row("Last frame");
    if (hasData)
    {
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%I64u", current_counter.count);
      ImGui::TableSetColumnIndex(2);
      auto allocationSizeUnit = size_to_unit_table(current_counter.size);
      ImGui::Text("%.2f %s", compute_unit_type_size(current_counter.size, allocationSizeUnit), get_unit_name(allocationSizeUnit));
    }

    char strBuf[64];
    sprintf_s(strBuf, "Peak at frame %I64u", peak_counter.index);
    begin_selectable_row(strBuf);
    if (hasData)
    {
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%I64u", peak_counter.count);
      ImGui::TableSetColumnIndex(2);
      auto metricsPeaksUnit = size_to_unit_table(peak_counter.size);
      ImGui::Text("%.2f %s", compute_unit_type_size(peak_counter.size, metricsPeaksUnit), get_unit_name(metricsPeaksUnit));
    }

    sprintf_s(strBuf, "Peak buffer at frame %I64u", peak_buffer_counter.index);
    begin_selectable_row(strBuf);
    if (hasData)
    {
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%I64u", peak_buffer_counter.count);
      ImGui::TableSetColumnIndex(2);
      auto metricsPeaksUnit = size_to_unit_table(peak_buffer_counter.size);
      ImGui::Text("%.2f %s", compute_unit_type_size(peak_buffer_counter.size, metricsPeaksUnit), get_unit_name(metricsPeaksUnit));
    }

    begin_selectable_row("Total");
    ImGui::TableNextColumn();
    if (hasData)
    {
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%I64u", summary_counter.count);
      ImGui::TableSetColumnIndex(2);
      auto totalAllocationSizeUnit = size_to_unit_table(summary_counter.size);
      ImGui::Text("%.2f %s", compute_unit_type_size(summary_counter.size, totalAllocationSizeUnit),
        get_unit_name(totalAllocationSizeUnit));
    }

    // when no data is available tell this in a tooltip
    if (!hasData)
    {
      if (ImGuiTableColumnFlags_IsHovered &
          (ImGui::TableGetColumnFlags(0) | ImGui::TableGetColumnFlags(1) | ImGui::TableGetColumnFlags(2)))
      {
        ScopedTooltip tooltip;
        ScopedWarningTextStyle warningStyle;
        ImGui::Bullet();
        ImGui::TextUnformatted("No memory usage data available");
      }
    }

    ImGui::EndTable();
  }
}

void MetricsVisualizer::drawBufferMemoryEvent(const MetricsState::ActionInfo &event, uint64_t index)
{
  begin_event_row(event.frameIndex, index, event.timeIndex, event_type_name_table[static_cast<uint32_t>(event.type)]);
  ImGui::TableSetColumnIndex(3);
  auto szUnit = size_to_unit_table(event.size);
  ImGui::Text("%.2f %s", compute_unit_type_size(event.size, szUnit), get_unit_name(szUnit));
  ImGui::TableSetColumnIndex(5);
  ImGui::TextUnformatted(event.isGPUMemory ? "GPU" : "CPU");
  ImGui::TableSetColumnIndex(6);
  ImGui::TextUnformatted(event.objectName.data(), event.objectName.data() + event.objectName.length());
  ImGui::TableSetColumnIndex(7);
  ImGui::TextUnformatted(event.eventPath.data(), event.eventPath.data() + event.eventPath.length());
}

void MetricsVisualizer::drawTextureEvent(const MetricsState::ActionInfo &event, uint64_t index)
{
  begin_event_row(event.frameIndex, index, event.timeIndex, event_type_name_table[static_cast<uint32_t>(event.type)]);
  ImGui::TableSetColumnIndex(3);
  if (event.size > 0)
  {
    auto szUnits = size_to_unit_table(event.size);
    ImGui::Text("%.2f %s", compute_unit_type_size(event.size, szUnits), get_unit_name(szUnits));
  }
  ImGui::TableSetColumnIndex(4);
  ImGui::Text("%u x %u x %u | %u mips | %u arrays", event.extent.width, event.extent.height, event.extent.depth, event.mips,
    event.arrays);
  ImGui::TableSetColumnIndex(5);
  // +12 skips over "DXGI_FORMAT_"
  ImGui::TextUnformatted(dxgi_format_name(event.format) + 12);
  ImGui::TableSetColumnIndex(6);
  ImGui::TextUnformatted(event.objectName.data(), event.objectName.data() + event.objectName.length());
  ImGui::TableSetColumnIndex(7);
  ImGui::TextUnformatted(event.eventPath.data(), event.eventPath.data() + event.eventPath.length());
}

void MetricsVisualizer::drawHostSharedMemoryEvent(const MetricsState::ActionInfo &event, uint64_t index)
{
  begin_event_row(event.frameIndex, index, event.timeIndex, event_type_name_table[static_cast<uint32_t>(event.type)]);
  ImGui::TableSetColumnIndex(3);
  auto szUnit = size_to_unit_table(event.size);
  ImGui::Text("%.2f %s", compute_unit_type_size(event.size, szUnit), get_unit_name(szUnit));
  ImGui::TableSetColumnIndex(5);
  ImGui::TableSetColumnIndex(6);
  ImGui::TextUnformatted(event.objectName.data(), event.objectName.data() + event.objectName.length());
  ImGui::TableSetColumnIndex(7);
  ImGui::TextUnformatted(event.eventPath.data(), event.eventPath.data() + event.eventPath.length());
}

bool MetricsVisualizer::drawEvent(const MetricsState::ActionInfo &event, uint64_t index)
{
  if (!isShownMetric(event_to_metric_type_table[static_cast<uint32_t>(event.type)]))
  {
    return false;
  }
  if (!getShownMetricFrameRange().isInside(event.frameIndex))
  {
    return false;
  }

  if (filter_object_name(event.objectName, getEventObjectNameFilterBasePointer()))
  {
    return false;
  }

  switch (event.type)
  {
    case MetricsState::ActionInfo::Type::ALLOCATE_HEAP:
    case MetricsState::ActionInfo::Type::FREE_HEAP:
    case MetricsState::ActionInfo::Type::ALLOCATE_MEMORY:
    case MetricsState::ActionInfo::Type::FREE_MEMORY:
    case MetricsState::ActionInfo::Type::ALLOCATE_BUFFER_HEAP:
    case MetricsState::ActionInfo::Type::FREE_BUFFER_HEAP:
    case MetricsState::ActionInfo::Type::ALLOCATE_USER_RESOURCE_HEAP:
    case MetricsState::ActionInfo::Type::FREE_USER_RESOURCE_HEAP:
    case MetricsState::ActionInfo::Type::PLACE_BUFFER_HEAP_IN_USER_RESOURCE_HEAP: drawBufferMemoryEvent(event, index); break;
    case MetricsState::ActionInfo::Type::ALLOCATE_TEXTURE:
    case MetricsState::ActionInfo::Type::ALIAS_TEXTURE:
    case MetricsState::ActionInfo::Type::ADOPT_TEXTURE:
    case MetricsState::ActionInfo::Type::FREE_TEXTURE:
    case MetricsState::ActionInfo::Type::PLACE_TEXTURE_IN_USER_RESOURCE_HEAP: drawTextureEvent(event, index); break;
    case MetricsState::ActionInfo::Type::ALLOCATE_CONST_RING:
    case MetricsState::ActionInfo::Type::FREE_CONST_RING:
    case MetricsState::ActionInfo::Type::ALLOCATE_TEMP_RING:
    case MetricsState::ActionInfo::Type::FREE_TEMP_RING:
    case MetricsState::ActionInfo::Type::ALLOCATE_TEMP_BUFFER:
    case MetricsState::ActionInfo::Type::FREE_TEMP_BUFFER:
    case MetricsState::ActionInfo::Type::USE_TEMP_BUFFER:
    case MetricsState::ActionInfo::Type::ALLOCATE_PERSISTENT_UPLOAD_BUFFER:
    case MetricsState::ActionInfo::Type::FREE_PERSISTENT_UPLOAD_BUFFER:
    case MetricsState::ActionInfo::Type::ALLOCATE_PERSISTENT_UPLOAD_MEMORY:
    case MetricsState::ActionInfo::Type::FREE_PERSISTENT_UPLOAD_MEMORY:
    case MetricsState::ActionInfo::Type::ALLOCATE_PERSISTENT_READ_BACK_BUFFER:
    case MetricsState::ActionInfo::Type::FREE_PERSISTENT_READ_BACK_BUFFER:
    case MetricsState::ActionInfo::Type::ALLOCATE_PERSISTENT_READ_BACK_MEMORY:
    case MetricsState::ActionInfo::Type::FREE_PERSISTENT_READ_BACK_MEMORY:
    case MetricsState::ActionInfo::Type::ALLOCATE_PERSISTENT_BIDIRECTIONAL_BUFFER:
    case MetricsState::ActionInfo::Type::FREE_PERSISTENT_BIDIRECTIONAL_BUFFER:
    case MetricsState::ActionInfo::Type::ALLOCATE_PERSISTENT_BIDIRECTIONAL_MEMORY:
    case MetricsState::ActionInfo::Type::FREE_PERSISTENT_BIDIRECTIONAL_MEMORY:
    case MetricsState::ActionInfo::Type::SCRATCH_BUFFER_ALLOCATE:
    case MetricsState::ActionInfo::Type::SCRATCH_BUFFER_FREE:
    case MetricsState::ActionInfo::Type::RAYTRACE_ACCEL_STRUCT_HEAP_ALLOCATE:
    case MetricsState::ActionInfo::Type::RAYTRACE_ACCEL_STRUCT_HEAP_FREE:
    case MetricsState::ActionInfo::Type::RAYTRACE_BOTTOM_STRUCTURE_ALLOCATE:
    case MetricsState::ActionInfo::Type::RAYTRACE_BOTTOM_STRUCTURE_FREE:
    case MetricsState::ActionInfo::Type::RAYTRACE_TOP_STRUCTURE_ALLOCATE:
    case MetricsState::ActionInfo::Type::RAYTRACE_TOP_STRUCTURE_FREE: drawHostSharedMemoryEvent(event, index); break;
    default: break;
  }
  return true;
}

MetricsVisualizer::GraphDisplayInfo MetricsVisualizer::drawGraphViewControls(Graph graph)
{
  GraphDisplayInfo result = getGraphDisplayInfo(graph);

  auto m = static_cast<int>(result.mode);
  ImGui::Combo("Graph Mode", &m, graph_mode_name_table, countof(graph_mode_name_table));
  result.mode = static_cast<GraphMode>(m);
  ImGui::SliderScalar("Window size", ImGuiDataType_U64, &result.windowSize, &min_graph_window_size, &max_graph_window_size, nullptr,
    ImGuiSliderFlags_AlwaysClamp);
  setGraphDisplayInfo(graph, result);
  return result;
}

void MetricsVisualizer::setupPlotY2CountRange(uint64_t max_value, bool has_data)
{
  if (max_value > 0 && has_data)
  {
    constexpr uint32_t yAxisPointCount = 11;
    constexpr uint32_t yAxisStringLength = 64;
    double yAxisPointValues[yAxisPointCount] = {};
    char yAxisPintStrings[yAxisPointCount][yAxisStringLength] = {};
    const char *yAxisPintStringPointers[yAxisPointCount];
    // 0 is a empty string, no need to print 0.00 <units>
    yAxisPintStrings[0][0] = '\0';
    yAxisPintStringPointers[0] = yAxisPintStrings[0];
    for (uint32_t i = 1; i < yAxisPointCount; ++i)
    {
      uint64_t value = max_value * i / (yAxisPointCount - 1);
      sprintf_s(yAxisPintStrings[i], "%I64u", value);
      yAxisPintStrings[i][yAxisStringLength - 1] = '\0';
      yAxisPintStringPointers[i] = yAxisPintStrings[i];
      yAxisPointValues[i] = value;
    }
    ImPlot::SetupAxisLimits(ImAxis_Y2, 0, max_value, ImGuiCond_Always);
    ImPlot::SetupAxisTicks(ImAxis_Y2, yAxisPointValues, yAxisPointCount, yAxisPintStringPointers, false);
  }
  else
  {
    const char *axisNames[2] = {"", ""};
    ImPlot::SetupAxesLimits(0, 1, 0, 1, ImGuiCond_Always);
    ImPlot::SetupAxisTicks(ImAxis_X1, 0, 1, 2, axisNames);
    ImPlot::SetupAxisTicks(ImAxis_Y2, 0, 1, 2, axisNames, false);
  }
}

void MetricsVisualizer::setupPlotYMemoryRange(uint64_t max_value, bool has_data)
{
  if (max_value > 0 && has_data)
  {
    constexpr uint32_t yAxisPointCount = 11;
    constexpr uint32_t yAxisStringLength = 64;
    double yAxisPointValues[yAxisPointCount] = {};
    char yAxisPintStrings[yAxisPointCount][yAxisStringLength] = {};
    const char *yAxisPintStringPointers[yAxisPointCount];
    auto valueUnits = size_to_unit_table(max_value);
    auto valueUnitsName = get_unit_name(valueUnits);
    // 0 is a empty string, no need to print 0.00 <units>
    yAxisPintStrings[0][0] = '\0';
    yAxisPintStringPointers[0] = yAxisPintStrings[0];
    for (uint32_t i = 1; i < yAxisPointCount; ++i)
    {
      uint64_t value = max_value * i / (yAxisPointCount - 1);
      sprintf_s(yAxisPintStrings[i], "%.2f %s", compute_unit_type_size(value, valueUnits), valueUnitsName);
      yAxisPintStrings[i][yAxisStringLength - 1] = '\0';
      yAxisPintStringPointers[i] = yAxisPintStrings[i];
      yAxisPointValues[i] = value;
    }
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, max_value, ImGuiCond_Always);
    ImPlot::SetupAxisTicks(ImAxis_Y1, yAxisPointValues, yAxisPointCount, yAxisPintStringPointers);
  }
  else
  {
    const char *axisNames[2] = {"", ""};
    ImPlot::SetupAxesLimits(0, 1, 0, 1, ImGuiCond_Always);
    ImPlot::SetupAxisTicks(ImAxis_X1, 0, 1, 2, axisNames);
    ImPlot::SetupAxisTicks(ImAxis_Y1, 0, 1, 2, axisNames);
  }
}

MetricsVisualizer::PlotData MetricsVisualizer::setupPlotXRange(ConcurrentMetricsStateAccessToken &access,
  const GraphDisplayInfo &display_state, bool has_data)
{
  PlotData result{access, {0, access->getMetricsHistoryLength()}};

  if (has_data && GraphMode::DISABLED != display_state.mode)
  {
    if (GraphMode::FULL_RANGE != display_state.mode)
    {
      if (GraphMode::BLOCK_SCROLLING == display_state.mode)
      {
        const auto length = result.getViewSize();
        if (length > 0)
        {
          auto newStart = (length - 1) - ((length - 1) % display_state.windowSize);
          auto newLength = (length - 1) - newStart + 1; // -V1065
          result.resetViewRange(newStart, newStart + newLength);
        }
      }
      else if (GraphMode::CONTIGUOUS_SCROLLING == display_state.mode)
      {
        if (result.getViewSize() > display_state.windowSize)
        {
          auto newStart = result.getViewSize() - display_state.windowSize;
          auto newLength = display_state.windowSize + 1;
          result.resetViewRange(newStart, newStart + newLength);
        }
      }
      const auto &first = access->getFrameMetrics(result.getFirstViewIndex());
      ImPlot::SetupAxisLimits(ImAxis_X1, first.frameIndex, first.frameIndex + display_state.windowSize, ImGuiCond_Always);
    }
  }
  else
  {
    result.resetViewRange(0, 0);
  }

  return result;
}
#endif

#if DX12_SUPPORT_RESOURCE_MEMORY_METRICS
void MetricsVisualizer::drawMetricsCaptureControls()
{
  constexpr int max_selectors_per_row = 5;
  constexpr int child_element_count = ((countof(metric_name_table) + max_selectors_per_row - 1) / max_selectors_per_row);
  int child_height = (child_element_count + 2) * ImGui::GetFrameHeightWithSpacing();

  if (begin_sub_section("DX12-Live-Metrics-Capture-Controls", "Capture metrics", child_height))
  {
    if (ImGui::BeginTable("DX12-Live-Metrics-Capture-Controls-Table", max_selectors_per_row, ImGuiTableFlags_NoClip))
    {
      for (auto &&e : metric_name_table)
      {
        ImGui::TableNextColumn();
        bool value = isCollectingMetric(e.metric);
        ImGui::Checkbox(e.name, &value);
        setCollectingMetric(e.metric, value);
      }
    }
    ImGui::EndTable();
  }
  end_sub_section();

  auto metricsAccess = accessMetrics();
  auto countersSize = metricsAccess->calculateMetricsMemoryUsage();
  auto countersSizeUnits = size_to_unit_table(countersSize);

  ImGui::Text("Using %.2f %s for events, metrics and history", compute_unit_type_size(countersSize, countersSizeUnits),
    get_unit_name(countersSizeUnits));
}

void MetricsVisualizer::drawMetricsEventsView()
{
  constexpr int table_height = 25;
  int child_height = (table_height + 3) * ImGui::GetFrameHeightWithSpacing();
  if (begin_sub_section("DX12-Live-Metrics-Event-View", "Events view", child_height))
  {
    auto metricsAccess = accessMetrics();
    uint64_t eventsShown = 0;
    if (ImGui::BeginTable("DX12-Live-Metrics-Event-Table", 8,
          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
          ImVec2(0, table_height * ImGui::GetFrameHeightWithSpacing())))
    {
      ImGui::TableSetupScrollFreeze(0, 1);
      make_table_header({"Frame", "Time", "Event", "Size", "", "", "Object Name", "Location"});

      metricsAccess->iterateMetricsActionLog([this, &eventsShown](const MetricsState::ActionInfo &event) //
        {
          if (drawEvent(event, eventsShown))
          {
            ++eventsShown;
          }
        });
      ImGui::EndTable();
    }
    uint64_t totalEvents = metricsAccess->getMetricsActionLogLength();
    char overlayText[64];
    float faction = 0.f;
    if (totalEvents > 0)
    {
      faction = float(eventsShown) / totalEvents;
      sprintf_s(overlayText, "%I64u / %I64u events show - %.2f%%", eventsShown, totalEvents, faction * 100.f);
    }
    else
    {
      strcpy(overlayText, "No events to view");
    }
    ImGui::ProgressBar(faction, ImVec2(-FLT_MIN, 0), overlayText);
  }
  end_sub_section();
}

void MetricsVisualizer::drawMetricsEvnetsViewFilterControls()
{
  constexpr int non_filter_metrics = 3;
  constexpr int max_selectors_per_row = 5;
  constexpr int child_element_count =
    ((countof(metric_name_table) - non_filter_metrics + max_selectors_per_row - 1) / max_selectors_per_row);
  int child_height = (child_element_count + 6) * ImGui::GetFrameHeightWithSpacing();

  if (begin_sub_section("DX12-Live-Metrics-Event-Filter-Controls", "Events view filters", child_height))
  {
    auto metricsAccess = accessMetrics();
    if (ImGui::BeginTable("DX12-Live-Metrics-Event-Filter-Table", max_selectors_per_row, ImGuiTableFlags_NoClip))
    {
      for (auto &&e : metric_name_table)
      {
        if ((Metric::EVENT_LOG == e.metric) || (Metric::METRIC_LOG == e.metric) || (Metric::MEMORY_USE == e.metric))
        {
          continue;
        }
        ImGui::TableNextColumn();
        bool value = isShownMetric(e.metric);
        ImGui::Checkbox(e.name, &value);
        setShownMetric(e.metric, value);
      }
      ImGui::EndTable();
    }
    uint64_t frameRangeMin = 0;
    uint64_t frameRangeMax = metricsAccess->getMetricsFrameMax();
    auto range = getShownMetricFrameRange();
    if (range.back() + 1 >= frameRangeMax || checkStatusFlag(StatusFlag::PIN_MAX_EVENT_FRAME_RANGE_TO_MAX))
    {
      range.reset(range.front(), frameRangeMax + 1);
    }
    if (range.front() + 1 >= frameRangeMax || checkStatusFlag(StatusFlag::PIN_MIN_EVENT_FRAME_RANGE_TO_MAX))
    {
      range.reset(frameRangeMax, range.back() + 1);
    }
    if (!range.isValidRange())
    {
      range.reset(range.front(), range.front() + 1);
    }
    uint64_t frameRange[2] = {range.front(), range.back()};
    ImGui::TextUnformatted("Frame range filter min / max. Setting a slider to max will pin it to "
                           "max value, so pinning from will only show last frame");
    // this will span the two sliders across the whole area instead of 2/3 or so
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::SliderScalarN("", ImGuiDataType_U64, frameRange, 2, &frameRangeMin, &frameRangeMax, nullptr);
    range.reset(frameRange[0], frameRange[1] + 1);
    setShownMetricFrameRange(range);
    setStatusFlag(StatusFlag::PIN_MIN_EVENT_FRAME_RANGE_TO_MAX, frameRange[0] >= frameRangeMax);
    setStatusFlag(StatusFlag::PIN_MAX_EVENT_FRAME_RANGE_TO_MAX, frameRange[1] >= frameRangeMax);
    ImGui::TextUnformatted("Object name filter");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputText("##DX12-Live-Metrics-Event-Object-Name-Filter", getEventObjectNameFilterBasePointer(),
          getEventObjectNameFilterMaxLength()))
    {
      setStatusFlag(StatusFlag::STORE_VIEWS_TO_CONFIG, true);
    }
    if (ImGui::Button("Clear log"))
    {
      clearMetricsActionLog();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear all"))
    {
      clearMetrics();
    }
  }
  end_sub_section();
}

void MetricsVisualizer::drawSystemCurrentMemoryUseTable()
{
  auto metricsAccess = accessMetrics();
  const auto &frameCounters = metricsAccess->getLastFrameMetrics();
  if (
    ImGui::BeginTable("DX12-Memory-Usage-Info-Table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    make_table_header( //
      {"Memory type", "Budget", "Available for reservation", "Current reservation", "Current usage"});

    bool rowTooltipShown = false;

#if _TARGET_PC_WIN
    begin_selectable_row("Device local");
    if (ImGui::IsItemHovered())
    {
      rowTooltipShown = true;
      ScopedTooltip tooltip;
      ImGui::TextUnformatted("The grouping of segments which is considered local to the video "
                             "adapter, and represents the fastest available memory to the GPU. "
                             "Applications should target the local segment group as the target "
                             "size for their working set.");
      if (!frameCounters.deviceMemoryInUse.CurrentUsage)
      {
        ScopedWarningTextStyle warningStyle;
        ImGui::Bullet();
        ImGui::TextUnformatted("No memory usage data available");
      }
    }
    ImGui::TableNextColumn();
    if (frameCounters.deviceMemoryInUse.Budget > 0)
    {
      auto budgetUnits = size_to_unit_table(frameCounters.deviceMemoryInUse.Budget);
      ImGui::Text("%.2f %s", compute_unit_type_size(frameCounters.deviceMemoryInUse.Budget, budgetUnits), get_unit_name(budgetUnits));
    }
    ImGui::TableNextColumn();
    if (frameCounters.deviceMemoryInUse.AvailableForReservation > 0)
    {
      auto availableForReservationUnits = size_to_unit_table(frameCounters.deviceMemoryInUse.AvailableForReservation);
      ImGui::Text("%.2f %s",
        compute_unit_type_size(frameCounters.deviceMemoryInUse.AvailableForReservation, availableForReservationUnits),
        get_unit_name(availableForReservationUnits));
    }
    ImGui::TableNextColumn();
    if (frameCounters.deviceMemoryInUse.CurrentReservation > 0)
    {
      auto currentReservationUnits = size_to_unit_table(frameCounters.deviceMemoryInUse.CurrentReservation);
      ImGui::Text("%.2f %s", compute_unit_type_size(frameCounters.deviceMemoryInUse.CurrentReservation, currentReservationUnits),
        get_unit_name(currentReservationUnits));
    }
    ImGui::TableNextColumn();
    if (frameCounters.deviceMemoryInUse.CurrentUsage > 0)
    {
      auto currentUsageUnits = size_to_unit_table(frameCounters.deviceMemoryInUse.CurrentUsage);
      ImGui::Text("%.2f %s", compute_unit_type_size(frameCounters.deviceMemoryInUse.CurrentUsage, currentUsageUnits),
        get_unit_name(currentUsageUnits));
    }

    begin_selectable_row("System shared");
    if (ImGui::IsItemHovered())
    {
      rowTooltipShown = true;
      ScopedTooltip tooltip;
      ImGui::TextUnformatted("The grouping of segments which is considered non-local to the video "
                             "adapter, and may have slower performance than the local segment "
                             "group.");
      if (!frameCounters.systemMemoryInUse.CurrentUsage)
      {
        ScopedWarningTextStyle warningStyle;
        ImGui::Bullet();
        ImGui::TextUnformatted("No memory usage data available");
      }
    }
    ImGui::TableNextColumn();
    if (frameCounters.systemMemoryInUse.Budget > 0)
    {
      auto budgetUnits = size_to_unit_table(frameCounters.systemMemoryInUse.Budget);
      ImGui::Text("%.2f %s", compute_unit_type_size(frameCounters.systemMemoryInUse.Budget, budgetUnits), get_unit_name(budgetUnits));
    }
    ImGui::TableNextColumn();
    if (frameCounters.systemMemoryInUse.AvailableForReservation > 0)
    {
      auto availableForReservationUnits = size_to_unit_table(frameCounters.systemMemoryInUse.AvailableForReservation);
      ImGui::Text("%.2f %s",
        compute_unit_type_size(frameCounters.systemMemoryInUse.AvailableForReservation, availableForReservationUnits),
        get_unit_name(availableForReservationUnits));
    }
    ImGui::TableNextColumn();
    if (frameCounters.systemMemoryInUse.CurrentReservation > 0)
    {
      auto currentReservationUnits = size_to_unit_table(frameCounters.systemMemoryInUse.CurrentReservation);
      ImGui::Text("%.2f %s", compute_unit_type_size(frameCounters.systemMemoryInUse.CurrentReservation, currentReservationUnits),
        get_unit_name(currentReservationUnits));
    }
    ImGui::TableNextColumn();
    if (frameCounters.systemMemoryInUse.CurrentUsage > 0)
    {
      auto currentUsageUnits = size_to_unit_table(frameCounters.systemMemoryInUse.CurrentUsage);
      ImGui::Text("%.2f %s", compute_unit_type_size(frameCounters.systemMemoryInUse.CurrentUsage, currentUsageUnits),
        get_unit_name(currentUsageUnits));
    }
#endif

    begin_selectable_row("Process");
    if (ImGui::IsItemHovered())
    {
      rowTooltipShown = true;
      ScopedTooltip tooltip;
      ImGui::TextUnformatted("The Commit Charge value for this process. Commit Charge is the total "
                             "amount of private memory that the memory manager has committed for a "
                             "running process.");
      ScopedWarningTextStyle warningStyle;
      if (frameCounters.processMemoryInUse > 0)
      {
        ImGui::Bullet();
        ImGui::TextUnformatted("Only current usage available");
      }
      else
      {
        ImGui::Bullet();
        ImGui::TextUnformatted("No memory usage data available");
      }
    }
    ImGui::TableNextColumn();
    // no budget
    ImGui::TableNextColumn();
    // no reservation
    ImGui::TableNextColumn();
    // no reservation
    ImGui::TableNextColumn();
    if (frameCounters.processMemoryInUse > 0)
    {
      auto processMemoryInUseUnits = size_to_unit_table(frameCounters.processMemoryInUse);
      ImGui::Text("%.2f %s", compute_unit_type_size(frameCounters.processMemoryInUse, processMemoryInUseUnits),
        get_unit_name(processMemoryInUseUnits));
    }

    if (ImGuiTableColumnFlags_IsHovered & ImGui::TableGetColumnFlags(1) && !rowTooltipShown)
    {
      ScopedTooltip tooltip;
      ImGui::TextUnformatted("Specifies the OS-provided video memory budget, that the "
                             "application should target. If current usage is greater than Budget, "
                             "the application may incur stuttering or performance penalties due to "
                             "background activity by the OS to provide other applications with a "
                             "fair usage of video memory.");
    }
    else if (ImGuiTableColumnFlags_IsHovered & ImGui::TableGetColumnFlags(2) && !rowTooltipShown)
    {
      ScopedTooltip tooltip;
      ImGui::TextUnformatted("The amount of video memory, that the application has "
                             "available for reservation.");
    }
    else if (ImGuiTableColumnFlags_IsHovered & ImGui::TableGetColumnFlags(3) && !rowTooltipShown)
    {
      ScopedTooltip tooltip;
      ImGui::TextUnformatted("The amount of video memory, that is reserved by the "
                             "application. The OS uses the reservation as a hint to determine the "
                             "applications minimum working set. Applications should attempt to "
                             "ensure that their video memory usage can be trimmed to meet this "
                             "requirement.");
    }
    else if (ImGuiTableColumnFlags_IsHovered & ImGui::TableGetColumnFlags(4) && !rowTooltipShown)
    {
      ScopedTooltip tooltip;
      ImGui::TextUnformatted("Specifies the applications current video memory usage.");
    }
    ImGui::EndTable();
  }
}

void MetricsVisualizer::drawSystemMemoryUsePlot()
{
  const int child_height = 17 * ImGui::GetFrameHeightWithSpacing();
  if (begin_sub_section("DX12-Memory-Usage-Graph-Segment", "Usage", child_height))
  {
    auto metricsAccess = accessMetrics();
    const auto &frame = metricsAccess->getLastFrameMetrics();
    uint64_t maxValue = frame.processMemoryInUsePeak;
#if _TARGET_PC_WIN
    maxValue = max(maxValue, frame.deviceMemoryInUsePeak.CurrentUsage);
    maxValue = max(maxValue, frame.systemMemoryInUsePeak.CurrentUsage);
#endif

    MetricsVisualizer::GraphDisplayInfo displayInfo = drawGraphViewControls(Graph::SYSTEM);

    if (ImPlot::BeginPlot("Memory usage over time##DX12-Memory-Usage-Graph", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend, ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit,
          ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
      auto plotData = setupPlotXRange(metricsAccess, displayInfo, maxValue > 0);
      setupPlotYMemoryRange(maxValue, plotData.hasAnyData());

      if (plotData.hasAnyData())
      {
        struct GetProcessMemoryValue
        {
          static double get(const PerFrameData &frame) { return double(frame.processMemoryInUse); }
        };
#if _TARGET_PC_WIN
        struct GetDeviceMemoryValue
        {
          static double get(const PerFrameData &frame) { return double(frame.deviceMemoryInUse.CurrentUsage); }
        };
        struct GetSystemMemoryValue
        {
          static double get(const PerFrameData &frame) { return double(frame.systemMemoryInUse.CurrentUsage); }
        };
#endif
        {
          ScopedPlotStyleVar shadedAlpha{ImPlotStyleVar_FillAlpha, 0.25f};
          plot_shaded("Process", getPlotPointFrameValue<ImPlotPoint, GetProcessMemoryValue>, getPlotPointFrameBase<ImPlotPoint>,
            plotData);
#if _TARGET_PC_WIN
          plot_shaded("Device local", getPlotPointFrameValue<ImPlotPoint, GetDeviceMemoryValue>, getPlotPointFrameBase<ImPlotPoint>,
            plotData);
          if (!isUMASystem())
          {
            plot_shaded("System shared", getPlotPointFrameValue<ImPlotPoint, GetSystemMemoryValue>, getPlotPointFrameBase<ImPlotPoint>,
              plotData);
          }
#endif
        }

        plot_line("Process", getPlotPointFrameValue<ImPlotPoint, GetProcessMemoryValue>, plotData);
#if _TARGET_PC_WIN
        plot_line("Device local", getPlotPointFrameValue<ImPlotPoint, GetDeviceMemoryValue>, plotData);
        if (!isUMASystem())
        {
          plot_line("System shared", getPlotPointFrameValue<ImPlotPoint, GetSystemMemoryValue>, plotData);
        }
#endif

        // When hovering the plot we show a tooltip with frame index and the sizes of each plot
        if (ImPlot::IsPlotHovered())
        {
          // those are actually the values in the graph where the mouse is pointing at
          auto pos = ImPlot::GetPlotMousePos();
          if (pos.x >= 0)
          {
            auto frameIdx = static_cast<uint64_t>(round(pos.x));
            if (plotData.selectFrame(frameIdx))
            {
              const auto &frame = plotData.getSelectedFrame();

              ScopedTooltip tooltip;
              ImGui::Text("Frame: %I64u", frameIdx);

              auto processMemoryInUseFrameUnits = size_to_unit_table(frame.processMemoryInUse);
              ImPlot::ItemIcon(ImPlot::GetColormapColor(0));
              ImGui::SameLine();
              ImGui::Text("Process: %.2f %s", compute_unit_type_size(frame.processMemoryInUse, processMemoryInUseFrameUnits),
                get_unit_name(processMemoryInUseFrameUnits));

#if _TARGET_PC_WIN
              auto deviceMemoryInUseFrameUnits = size_to_unit_table(frame.deviceMemoryInUse.CurrentUsage);
              ImPlot::ItemIcon(ImPlot::GetColormapColor(1));
              ImGui::SameLine();
              ImGui::Text("Device: %.2f %s", compute_unit_type_size(frame.deviceMemoryInUse.CurrentUsage, deviceMemoryInUseFrameUnits),
                get_unit_name(deviceMemoryInUseFrameUnits));

              if (!isUMASystem())
              {
                auto systemMemoryInUseFrameUnits = size_to_unit_table(frame.systemMemoryInUse.CurrentUsage);
                ImPlot::ItemIcon(ImPlot::GetColormapColor(2));
                ImGui::SameLine();
                ImGui::Text("System: %.2f %s",
                  compute_unit_type_size(frame.systemMemoryInUse.CurrentUsage, systemMemoryInUseFrameUnits),
                  get_unit_name(systemMemoryInUseFrameUnits));
              }
#endif
            }
          }
        }
      }
      else
      {
        // When no data is available just display a empty plot
        ImPlot::PlotDummy("Process");
#if _TARGET_PC_WIN
        ImPlot::PlotDummy("Device local");
        if (!isUMASystem())
        {
          ImPlot::PlotDummy("System shared");
        }
#endif
        // Tell the user, there nothing to show in the plot
        ImPlot::PushStyleColor(ImPlotCol_InlayText, ImVec4(.75f, .75f, 0.f, 1.f));
        ImPlot::PlotText("No historical data available", 0.5, 0.5);
        ImPlot::PopStyleColor();
      }
      ImPlot::EndPlot();
    }
  }
  end_sub_section();
}

void MetricsVisualizer::drawHeapsPlot()
{
  const int child_height = 17 * ImGui::GetFrameHeightWithSpacing();
  if (begin_sub_section("DX12-Heap-Graph-Segment", "Usage", child_height))
  {
    auto metricsAccess = accessMetrics();
    const auto &frame = metricsAccess->getLastFrameMetrics();
    uint64_t maxY2 = frame.netCountersPeak.textures.count;
    maxY2 = max(maxY2, frame.netCountersPeak.gpuMemory.count);
    maxY2 = max(maxY2, frame.netCountersPeak.gpuHeaps.count);
    maxY2 = max(maxY2, frame.netCountersPeak.cpuMemory.count);
    maxY2 = max(maxY2, frame.netCountersPeak.cpuHeaps.count);

    MetricsVisualizer::GraphDisplayInfo displayInfo = drawGraphViewControls(Graph::HEAPS);

    if (ImPlot::BeginPlot("Usage##DX12-Heap-Graph", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend | ImPlotFlags_YAxis2,
          ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
      auto plotData = setupPlotXRange(metricsAccess, displayInfo, maxY2 > 0);
      // heap size will always be the largest
      setupPlotYMemoryRange(max(frame.netCountersPeak.gpuHeaps.size, frame.netCountersPeak.cpuHeaps.size), plotData.hasAnyData());
      setupPlotY2CountRange(maxY2, plotData.hasAnyData());

      if (plotData.hasAnyData())
      {
        struct GetGPUHeapSize
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.gpuHeaps.size); }
        };
        struct GetCPUHeapSize
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.cpuHeaps.size); }
        };
        struct GetGPUMemoryUsedSize
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.gpuMemory.size); }
        };
        struct GetCPUMemoryUsedSize
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.cpuMemory.size); }
        };
        struct GetGPUHeapCount
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.gpuHeaps.count); }
        };
        struct GetCPUHeapCount
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.gpuHeaps.count); }
        };
        struct GetGPUMemoryUsedCount
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.gpuMemory.count); }
        };
        struct GetCPUMemoryUsedCount
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.cpuMemory.count); }
        };
        {
          ScopedPlotStyleVar shadedAlpha{ImPlotStyleVar_FillAlpha, 0.25f};
          plot_shaded("GPU Heaps Size", getPlotPointFrameValue<ImPlotPoint, GetGPUHeapSize>, getPlotPointFrameBase<ImPlotPoint>,
            plotData);
          plot_shaded("CPU Heaps Size", getPlotPointFrameValue<ImPlotPoint, GetCPUHeapSize>, getPlotPointFrameBase<ImPlotPoint>,
            plotData);
          plot_shaded("GPU Allocated Size", getPlotPointFrameValue<ImPlotPoint, GetGPUMemoryUsedSize>,
            getPlotPointFrameBase<ImPlotPoint>, plotData);
          plot_shaded("CPU Allocated Size", getPlotPointFrameValue<ImPlotPoint, GetCPUMemoryUsedSize>,
            getPlotPointFrameBase<ImPlotPoint>, plotData);
        }

        plot_line("GPU Heaps Size", getPlotPointFrameValue<ImPlotPoint, GetGPUHeapSize>, plotData);
        plot_line("CPU Heaps Size", getPlotPointFrameValue<ImPlotPoint, GetCPUHeapSize>, plotData);
        plot_line("GPU Allocated Size", getPlotPointFrameValue<ImPlotPoint, GetGPUMemoryUsedSize>, plotData);
        plot_line("CPU Allocated Size", getPlotPointFrameValue<ImPlotPoint, GetCPUMemoryUsedSize>, plotData);

        ImPlot::SetAxis(ImAxis_Y2);

        plot_line("GPU Heap Count", getPlotPointFrameValue<ImPlotPoint, GetGPUHeapCount>, plotData);
        plot_line("CPU Heap Count", getPlotPointFrameValue<ImPlotPoint, GetCPUHeapCount>, plotData);
        plot_line("GPU Allocation Count", getPlotPointFrameValue<ImPlotPoint, GetGPUMemoryUsedCount>, plotData);
        plot_line("CPU Allocation Count", getPlotPointFrameValue<ImPlotPoint, GetCPUMemoryUsedCount>, plotData);

        // When hovering the plot we show a tooltip with frame index and the sizes of each plot
        if (ImPlot::IsPlotHovered())
        {
          // those are actually the values in the graph where the mouse is pointing at
          auto pos = ImPlot::GetPlotMousePos();
          if (pos.x >= 0)
          {
            auto frameIdx = static_cast<uint64_t>(round(pos.x));
            if (plotData.selectFrame(frameIdx))
            {
              const auto &frame = plotData.getSelectedFrame();

              ScopedTooltip tooltip;
              ImGui::Text("Frame: %I64u", frameIdx);

              auto gpuHeapSize = size_to_unit_table(frame.netCounters.gpuHeaps.size);
              ImPlot::ItemIcon(ImPlot::GetColormapColor(0));
              ImGui::SameLine();
              ImGui::Text("GPU Heaps Size: %.2f %s", compute_unit_type_size(frame.netCounters.gpuHeaps.size, gpuHeapSize),
                get_unit_name(gpuHeapSize));

              auto cpuHeapSize = size_to_unit_table(frame.netCounters.cpuHeaps.size);
              ImPlot::ItemIcon(ImPlot::GetColormapColor(1));
              ImGui::SameLine();
              ImGui::Text("CPU Heaps Size: %.2f %s", compute_unit_type_size(frame.netCounters.cpuHeaps.size, cpuHeapSize),
                get_unit_name(cpuHeapSize));

              auto gpuAllocatedSize = size_to_unit_table(frame.netCounters.gpuMemory.size);
              ImPlot::ItemIcon(ImPlot::GetColormapColor(2));
              ImGui::SameLine();
              ImGui::Text("GPU Allocated Size: %.2f %s", compute_unit_type_size(frame.netCounters.gpuMemory.size, gpuAllocatedSize),
                get_unit_name(gpuAllocatedSize));

              auto cpuAllocatedSize = size_to_unit_table(frame.netCounters.cpuMemory.size);
              ImPlot::ItemIcon(ImPlot::GetColormapColor(3));
              ImGui::SameLine();
              ImGui::Text("CPU Allocated Size: %.2f %s", compute_unit_type_size(frame.netCounters.cpuMemory.size, cpuAllocatedSize),
                get_unit_name(cpuAllocatedSize));

              ImPlot::ItemIcon(ImPlot::GetColormapColor(4));
              ImGui::SameLine();
              ImGui::Text("GPU Heap Count: %I64u", frame.netCounters.gpuHeaps.count);

              ImPlot::ItemIcon(ImPlot::GetColormapColor(5));
              ImGui::SameLine();
              ImGui::Text("CPU Heap Count: %I64u", frame.netCounters.cpuHeaps.count);

              ImPlot::ItemIcon(ImPlot::GetColormapColor(6));
              ImGui::SameLine();
              ImGui::Text("GPU Allocation Count: %I64u", frame.netCounters.gpuMemory.count);

              ImPlot::ItemIcon(ImPlot::GetColormapColor(7));
              ImGui::SameLine();
              ImGui::Text("CPU Allocation Count: %I64u", frame.netCounters.cpuMemory.count);
            }
          }
        }
      }
      else
      {
        // When no data is available just display a empty plot
        ImPlot::PlotDummy("GPU Heaps Size");
        ImPlot::PlotDummy("CPU Heaps Size");
        ImPlot::PlotDummy("GPU Allocated Size");
        ImPlot::PlotDummy("CPU Allocated Size");
        ImPlot::PlotDummy("GPU Heap Count");
        ImPlot::PlotDummy("CPU Heap Count");
        ImPlot::PlotDummy("GPU Allocation Count");
        ImPlot::PlotDummy("CPU Allocation Count");
        // Tell the user, there nothing to show in the plot
        ImPlot::PushStyleColor(ImPlotCol_InlayText, ImVec4(.75f, .75f, 0.f, 1.f));
        ImPlot::PlotText("No historical data available", 0.5, 0.5);
        ImPlot::PopStyleColor();
      }
      ImPlot::EndPlot();
    }
  }
  end_sub_section();
}

void MetricsVisualizer::drawHeapsSummaryTable()
{
  if (ImGui::BeginTable("DX12-Heap-Summary-Table", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    auto metricsAccess = accessMetrics();
    const auto &frame = metricsAccess->getLastFrameMetrics();

    make_table_header( //
      {"Segment", "Textures", "Texture Size", "Buffers", "Buffer Size", "Allocated Size", "Free Size", "Total Size"});
    begin_selectable_row("GPU");

    ImGui::TableNextColumn();
    ImGui::Text("%I64u", frame.netCounters.textures.count);

    ImGui::TableNextColumn();
    auto texutreSizeUnits = size_to_unit_table(frame.netCounters.textures.size);
    ImGui::Text("%.2f %s", compute_unit_type_size(frame.netCounters.textures.size, texutreSizeUnits), get_unit_name(texutreSizeUnits));

    ImGui::TableNextColumn();
    ImGui::Text("%I64u", frame.netCounters.gpuBufferHeaps.count);

    ImGui::TableNextColumn();
    auto bufferHeapSizeUnits = size_to_unit_table(frame.netCounters.gpuBufferHeaps.size);
    ImGui::Text("%.2f %s", compute_unit_type_size(frame.netCounters.gpuBufferHeaps.size, bufferHeapSizeUnits),
      get_unit_name(bufferHeapSizeUnits));

    ImGui::TableNextColumn();
    auto allocatedMemorySizeUnits = size_to_unit_table(frame.netCounters.gpuMemory.size);
    ImGui::Text("%.2f %s", compute_unit_type_size(frame.netCounters.gpuMemory.size, allocatedMemorySizeUnits),
      get_unit_name(allocatedMemorySizeUnits));

    ImGui::TableNextColumn();
    auto freeMemorySize = frame.netCounters.gpuHeaps.size - frame.netCounters.gpuMemory.size;
    auto freeMemorySizeUnits = size_to_unit_table(freeMemorySize);
    ImGui::Text("%.2f %s", compute_unit_type_size(freeMemorySize, freeMemorySizeUnits), get_unit_name(freeMemorySizeUnits));

    ImGui::TableNextColumn();
    auto heapSizeMemoryUnits = size_to_unit_table(frame.netCounters.gpuHeaps.size);
    ImGui::Text("%.2f %s", compute_unit_type_size(frame.netCounters.gpuHeaps.size, heapSizeMemoryUnits),
      get_unit_name(heapSizeMemoryUnits));

    begin_selectable_row("CPU");

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    auto totalCPUBufferHeapCount = frame.netCounters.cpuBufferHeaps.count;
    totalCPUBufferHeapCount += frame.netCounters.constRing.count;
    totalCPUBufferHeapCount += frame.netCounters.uploadRing.count;
    totalCPUBufferHeapCount += frame.netCounters.tempBuffer.count;
    totalCPUBufferHeapCount += frame.netCounters.persistentUploadBuffer.count;
    totalCPUBufferHeapCount += frame.netCounters.persistentReadBackBuffer.count;
    totalCPUBufferHeapCount += frame.netCounters.persistentBidirectionalBuffer.count;
    ImGui::Text("%I64u", totalCPUBufferHeapCount);

    ImGui::TableNextColumn();
    auto totalCPUBufferHeapSize = frame.netCounters.cpuBufferHeaps.size;
    totalCPUBufferHeapSize += frame.netCounters.constRing.size;
    totalCPUBufferHeapSize += frame.netCounters.uploadRing.size;
    totalCPUBufferHeapSize += frame.netCounters.tempBuffer.size;
    totalCPUBufferHeapSize += frame.netCounters.persistentUploadBuffer.size;
    totalCPUBufferHeapSize += frame.netCounters.persistentReadBackBuffer.size;
    totalCPUBufferHeapSize += frame.netCounters.persistentBidirectionalBuffer.size;
    bufferHeapSizeUnits = size_to_unit_table(totalCPUBufferHeapSize);
    ImGui::Text("%.2f %s", compute_unit_type_size(totalCPUBufferHeapSize, bufferHeapSizeUnits), get_unit_name(bufferHeapSizeUnits));

    ImGui::TableNextColumn();
    allocatedMemorySizeUnits = size_to_unit_table(frame.netCounters.cpuMemory.size);
    ImGui::Text("%.2f %s", compute_unit_type_size(frame.netCounters.cpuMemory.size, allocatedMemorySizeUnits),
      get_unit_name(allocatedMemorySizeUnits));

    ImGui::TableNextColumn();
    freeMemorySize = frame.netCounters.cpuHeaps.size - frame.netCounters.cpuMemory.size;
    freeMemorySizeUnits = size_to_unit_table(freeMemorySize);
    ImGui::Text("%.2f %s", compute_unit_type_size(freeMemorySize, freeMemorySizeUnits), get_unit_name(freeMemorySizeUnits));

    ImGui::TableNextColumn();
    heapSizeMemoryUnits = size_to_unit_table(frame.netCounters.cpuHeaps.size);
    ImGui::Text("%.2f %s", compute_unit_type_size(frame.netCounters.cpuHeaps.size, heapSizeMemoryUnits),
      get_unit_name(heapSizeMemoryUnits));

    begin_selectable_row("Total");

    ImGui::TableNextColumn();
    ImGui::Text("%I64u", frame.netCounters.textures.count);

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s", compute_unit_type_size(frame.netCounters.textures.size, texutreSizeUnits), get_unit_name(texutreSizeUnits));

    ImGui::TableNextColumn();
    ImGui::Text("%I64u", totalCPUBufferHeapCount + frame.netCounters.gpuBufferHeaps.count);

    ImGui::TableNextColumn();
    bufferHeapSizeUnits = size_to_unit_table(totalCPUBufferHeapSize + frame.netCounters.gpuBufferHeaps.size);
    ImGui::Text("%.2f %s", compute_unit_type_size(totalCPUBufferHeapSize + frame.netCounters.gpuBufferHeaps.size, bufferHeapSizeUnits),
      get_unit_name(bufferHeapSizeUnits));

    ImGui::TableNextColumn();
    allocatedMemorySizeUnits = size_to_unit_table(frame.netCounters.gpuMemory.size + frame.netCounters.cpuMemory.size);
    ImGui::Text("%.2f %s",
      compute_unit_type_size(frame.netCounters.gpuMemory.size + frame.netCounters.cpuMemory.size, allocatedMemorySizeUnits),
      get_unit_name(allocatedMemorySizeUnits));

    ImGui::TableNextColumn();
    freeMemorySize = frame.netCounters.gpuHeaps.size - frame.netCounters.gpuMemory.size + frame.netCounters.cpuHeaps.size -
                     frame.netCounters.cpuMemory.size;
    freeMemorySizeUnits = size_to_unit_table(freeMemorySize);
    ImGui::Text("%.2f %s", compute_unit_type_size(freeMemorySize, freeMemorySizeUnits), get_unit_name(freeMemorySizeUnits));

    ImGui::TableNextColumn();
    heapSizeMemoryUnits = size_to_unit_table(frame.netCounters.gpuHeaps.size + frame.netCounters.cpuHeaps.size);
    ImGui::Text("%.2f %s",
      compute_unit_type_size(frame.netCounters.gpuHeaps.size + frame.netCounters.cpuHeaps.size, heapSizeMemoryUnits),
      get_unit_name(heapSizeMemoryUnits));

#if _TARGET_PC_WIN
    begin_selectable_row("Overhead GPU");
    if (ImGui::IsItemHovered())
    {
      ScopedTooltip tooltip;
      ImGui::TextUnformatted("Difference between usage by all GPU memory heaps and OS reported GPU "
                             "memory use");
    }

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    heapSizeMemoryUnits = size_to_unit_table(frame.deviceMemoryInUse.CurrentUsage - frame.netCounters.gpuHeaps.size);
    ImGui::Text("%.2f %s",
      compute_unit_type_size(frame.deviceMemoryInUse.CurrentUsage - frame.netCounters.gpuHeaps.size, heapSizeMemoryUnits),
      get_unit_name(heapSizeMemoryUnits));

    begin_selectable_row("Overhead CPU");
    if (ImGui::IsItemHovered())
    {
      ScopedTooltip tooltip;
      ImGui::TextUnformatted("Difference between usage by all CPU memory heaps and OS reported CPU "
                             "memory use");
    }

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    heapSizeMemoryUnits = size_to_unit_table(frame.systemMemoryInUse.CurrentUsage - frame.netCounters.cpuHeaps.size);
    ImGui::Text("%.2f %s",
      compute_unit_type_size(frame.systemMemoryInUse.CurrentUsage - frame.netCounters.cpuHeaps.size, heapSizeMemoryUnits),
      get_unit_name(heapSizeMemoryUnits));
#endif

    ImGui::EndTable();
  }
}

void MetricsVisualizer::drawScratchBufferTable() {}

void MetricsVisualizer::drawScratchBufferPlot()
{
  const int child_height = 17 * ImGui::GetFrameHeightWithSpacing();
  if (begin_sub_section("DX12-Scratch-Buffer-Plot-Segment", "Usage", child_height))
  {
    auto metricsAccess = accessMetrics();
    const auto &frame = metricsAccess->getLastFrameMetrics();
    uint64_t maxY2 = frame.netCountersPeak.scratchBuffer.count;

    MetricsVisualizer::GraphDisplayInfo displayInfo = drawGraphViewControls(Graph::SCRATCH_BUFFER);

    if (ImPlot::BeginPlot("Usage##DX12-Scratch-Buffer-Plot", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend | ImPlotFlags_YAxis2,
          ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
      auto plotData = setupPlotXRange(metricsAccess, displayInfo, maxY2 > 0);
      setupPlotYMemoryRange(frame.netCountersPeak.scratchBuffer.size, plotData.hasAnyData());
      setupPlotY2CountRange(maxY2, plotData.hasAnyData());

      if (plotData.hasAnyData())
      {
        struct GetBufferSize
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.scratchBuffer.size); }
        };
        struct GetTempUse
        {
          static double get(const PerFrameData &frame) { return double(frame.rawCounters.scratchBufferTempUse.size); }
        };
        struct GetPersistentUse
        {
          static double get(const PerFrameData &frame) { return double(frame.rawCounters.scratchBufferPersistentUse.size); }
        };
        struct GetBufferCount
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.scratchBuffer.count); }
        };
        {
          ScopedPlotStyleVar shadedAlpha{ImPlotStyleVar_FillAlpha, 0.25f};
          plot_shaded("Buffer Size", getPlotPointFrameValue<ImPlotPoint, GetBufferSize>, getPlotPointFrameBase<ImPlotPoint>, plotData);
        }

        plot_line("Buffer Size", getPlotPointFrameValue<ImPlotPoint, GetBufferSize>, plotData);
        plot_line("Temporary usage", getPlotPointFrameValue<ImPlotPoint, GetTempUse>, plotData);
        plot_line("Persistent usage", getPlotPointFrameValue<ImPlotPoint, GetPersistentUse>, plotData);

        ImPlot::SetAxis(ImAxis_Y2);

        plot_line("User Heap Count", getPlotPointFrameValue<ImPlotPoint, GetBufferCount>, plotData);

        // When hovering the plot we show a tooltip with frame index and the sizes of each plot
        if (ImPlot::IsPlotHovered())
        {
          // those are actually the values in the graph where the mouse is pointing at
          auto pos = ImPlot::GetPlotMousePos();
          if (pos.x >= 0)
          {
            auto frameIdx = static_cast<uint64_t>(round(pos.x));
            if (plotData.selectFrame(frameIdx))
            {
              const auto &frame = plotData.getSelectedFrame();

              ScopedTooltip tooltip;
              ImGui::Text("Frame: %I64u", frameIdx);

              ByteUnits bufferSize = frame.netCounters.scratchBuffer.size;
              ImPlot::ItemIcon(ImPlot::GetColormapColor(0));
              ImGui::SameLine();
              ImGui::Text("Scratch Buffer Size: %.2f %s", bufferSize.units(), bufferSize.name());

              ByteUnits tempUse = frame.rawCounters.scratchBufferTempUse.size;
              ImPlot::ItemIcon(ImPlot::GetColormapColor(1));
              ImGui::SameLine();
              ImGui::Text("Temporary Use: %.2f %s", tempUse.units(), tempUse.name());

              ByteUnits persistentUse = frame.rawCounters.scratchBufferPersistentUse.size;
              ImPlot::ItemIcon(ImPlot::GetColormapColor(2));
              ImGui::SameLine();
              ImGui::Text("Persistent Use: %.2f %s", tempUse.units(), persistentUse.name());

              ImPlot::ItemIcon(ImPlot::GetColormapColor(3));
              ImGui::SameLine();
              ImGui::Text("Scratch Buffer Count: %I64u", frame.netCounters.scratchBuffer.count);
            }
          }
        }
      }
      else
      {
        // When no data is available just display a empty plot
        ImPlot::PlotDummy("Scratch Buffer Size");
        ImPlot::PlotDummy("Scratch Buffer Count");
        // Tell the user, there nothing to show in the plot
        ImPlot::PushStyleColor(ImPlotCol_InlayText, ImVec4(.75f, .75f, 0.f, 1.f));
        ImPlot::PlotText("No historical data available", 0.5, 0.5);
        ImPlot::PopStyleColor();
      }
      ImPlot::EndPlot();
    }
  }
  end_sub_section();
}

void MetricsVisualizer::drawTexturePlot()
{
  const int child_height = 17 * ImGui::GetFrameHeightWithSpacing();
  if (begin_sub_section("DX12-Texture-Graph-Segment", "Usage", child_height))
  {
    auto metricsAccess = accessMetrics();
    // only two things, memory usage and object count
    const auto &frame = metricsAccess->getLastFrameMetrics();

    MetricsVisualizer::GraphDisplayInfo displayInfo = drawGraphViewControls(Graph::TEXTURES);

    if (ImPlot::BeginPlot("Usage##DX12-Texture-Graph", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend | ImPlotFlags_YAxis2,
          ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
      auto plotData = setupPlotXRange(metricsAccess, displayInfo, frame.netCountersPeak.textures.count);
      // heap size will always be the largest
      setupPlotYMemoryRange(frame.netCountersPeak.textures.size, plotData.hasAnyData());
      setupPlotY2CountRange(frame.netCountersPeak.textures.count, plotData.hasAnyData());

      if (plotData.hasAnyData())
      {
        struct GetSize
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.textures.size); }
        };
        struct GetCount
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.textures.count); }
        };
        {
          ScopedPlotStyleVar shadedAlpha{ImPlotStyleVar_FillAlpha, 0.25f};
          plot_shaded("Size", getPlotPointFrameValue<ImPlotPoint, GetSize>, getPlotPointFrameBase<ImPlotPoint>, plotData);
        }

        plot_line("Size", getPlotPointFrameValue<ImPlotPoint, GetSize>, plotData);

        ImPlot::SetAxis(ImAxis_Y2);

        plot_line("Count", getPlotPointFrameValue<ImPlotPoint, GetCount>, plotData);

        // When hovering the plot we show a tooltip with frame index and the sizes of each plot
        if (ImPlot::IsPlotHovered())
        {
          // those are actually the values in the graph where the mouse is pointing at
          auto pos = ImPlot::GetPlotMousePos();
          if (pos.x >= 0)
          {
            auto frameIdx = static_cast<uint64_t>(round(pos.x));
            if (plotData.selectFrame(frameIdx))
            {
              const auto &frame = plotData.getSelectedFrame();

              ScopedTooltip tooltip;
              ImGui::Text("Frame: %I64u", frameIdx);

              auto sizeUnits = size_to_unit_table(frame.netCounters.textures.size);
              ImPlot::ItemIcon(ImPlot::GetColormapColor(0));
              ImGui::SameLine();
              ImGui::Text("Size: %.2f %s", compute_unit_type_size(frame.netCounters.textures.size, sizeUnits),
                get_unit_name(sizeUnits));

              ImPlot::ItemIcon(ImPlot::GetColormapColor(1));
              ImGui::SameLine();
              ImGui::Text("Count: %I64u", frame.netCounters.textures.count);
            }
          }
        }
      }
      else
      {
        // When no data is available just display a empty plot
        ImPlot::PlotDummy("Size");
        ImPlot::PlotDummy("Count");
        // Tell the user, there nothing to show in the plot
        ImPlot::PushStyleColor(ImPlotCol_InlayText, ImVec4(.75f, .75f, 0.f, 1.f));
        ImPlot::PlotText("No historical data available", 0.5, 0.5);
        ImPlot::PopStyleColor();
      }
      ImPlot::EndPlot();
    }
  }
  end_sub_section();
}

void MetricsVisualizer::drawBuffersPlot()
{
  struct GetCPUMemory
  {
    static double get(const PerFrameData &frame) { return double(frame.netCounters.cpuBufferHeaps.size); }
  };
  struct GetGPUMemory
  {
    static double get(const PerFrameData &frame) { return double(frame.netCounters.gpuBufferHeaps.size); }
  };
  struct GetCPUBuffer
  {
    static double get(const PerFrameData &frame) { return double(frame.netCounters.cpuBufferHeaps.count); }
  };
  struct GetGPUBuffer
  {
    static double get(const PerFrameData &frame) { return double(frame.netCounters.gpuBufferHeaps.count); }
  };

  const int child_height = 17 * ImGui::GetFrameHeightWithSpacing();
  if (begin_sub_section("DX12-Buffer-Heap-Usage-Plot-Section", "Buffer Usage Plot", child_height))
  {
    auto metricsAccess = accessMetrics();
    const auto &frame = metricsAccess->getLastFrameMetrics();
    const bool hasData = frame.netCountersPeak.cpuBufferHeaps.size || frame.netCountersPeak.gpuBufferHeaps.size;
    MetricsVisualizer::GraphDisplayInfo displayInfo = drawGraphViewControls(Graph::BUFFERS);

    if (ImPlot::BeginPlot("Usage##DX12-Buffer-Heap-Usage-Plot", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend | ImPlotFlags_YAxis2,
          ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
      auto plotData = setupPlotXRange(metricsAccess, displayInfo, hasData);

      setupPlotYMemoryRange(max(frame.netCountersPeak.cpuBufferHeaps.size, frame.netCountersPeak.gpuBufferHeaps.size),
        plotData.hasAnyData());
      setupPlotY2CountRange(max(frame.netCountersPeak.cpuBufferHeaps.count, frame.netCountersPeak.gpuBufferHeaps.count),
        plotData.hasAnyData());

      if (plotData.hasAnyData())
      {
        {
          ScopedPlotStyleVar shadedAlpha{ImPlotStyleVar_FillAlpha, 0.25f};
          plot_shaded("CPU Memory", getPlotPointFrameValue<ImPlotPoint, GetCPUMemory>, getPlotPointFrameBase<ImPlotPoint>, plotData);
          plot_shaded("GPU Memory", getPlotPointFrameValue<ImPlotPoint, GetGPUMemory>, getPlotPointFrameBase<ImPlotPoint>, plotData);
        }
        plot_line("CPU Memory", getPlotPointFrameValue<ImPlotPoint, GetCPUMemory>, plotData);
        plot_line("GPU Memory", getPlotPointFrameValue<ImPlotPoint, GetGPUMemory>, plotData);

        ImPlot::SetAxis(ImAxis_Y2);
        plot_line("CPU Buffers", getPlotPointFrameValue<ImPlotPoint, GetCPUBuffer>, plotData);
        plot_line("GPU Buffers", getPlotPointFrameValue<ImPlotPoint, GetGPUBuffer>, plotData);

        // when hovering the plot we show the frame and the total buffer size and usage at that
        // frame
        if (ImPlot::IsPlotHovered())
        {
          // those are actually the values in the graph where the mouse is pointing at
          auto pos = ImPlot::GetPlotMousePos();
          if (pos.x >= 0)
          {
            auto frameIdx = static_cast<uint64_t>(round(pos.x));
            if (plotData.selectFrame(frameIdx))
            {
              const auto &frame = plotData.getSelectedFrame();
              ScopedTooltip tooltip;
              ImGui::Text("Frame: %I64u", frameIdx);

              auto allocatedValue = frame.netCounters.cpuBufferHeaps.size;
              auto allocatedValueUnits = size_to_unit_table(allocatedValue);
              ImPlot::ItemIcon(ImPlot::GetColormapColor(0));
              ImGui::SameLine();
              ImGui::Text("CPU Memory: %.2f %s", compute_unit_type_size(allocatedValue, allocatedValueUnits),
                get_unit_name(allocatedValueUnits));

              auto usedValue = frame.netCounters.gpuBufferHeaps.size;
              auto usedValueUnits = size_to_unit_table(usedValue);
              ImPlot::ItemIcon(ImPlot::GetColormapColor(1));
              ImGui::SameLine();
              ImGui::Text("GPU Memory: %.2f %s", compute_unit_type_size(usedValue, usedValueUnits), get_unit_name(usedValueUnits));

              ImPlot::ItemIcon(ImPlot::GetColormapColor(2));
              ImGui::SameLine();
              ImGui::Text("CPU Buffers: %I64u", frame.netCounters.cpuBufferHeaps.count);

              ImPlot::ItemIcon(ImPlot::GetColormapColor(3));
              ImGui::SameLine();
              ImGui::Text("GPU Buffers: %I64u", frame.netCounters.gpuBufferHeaps.count);
            }
          }
        }
      }
      else
      {
        // When no data is available just display a empty plot
        ImPlot::PlotDummy("CPU Memory");
        ImPlot::PlotDummy("GPU Memory");
        ImPlot::PlotDummy("CPU Buffers");
        ImPlot::PlotDummy("GPU Buffers");
        // Tell the user, there nothing to show in the plot
        ImPlot::PushStyleColor(ImPlotCol_InlayText, ImVec4(.75f, .75f, 0.f, 1.f));
        ImPlot::PlotText("No historical data available", 0.5, 0.5);
        ImPlot::PopStyleColor();
      }
      ImPlot::EndPlot();
    }
  }
  end_sub_section();
}

void MetricsVisualizer::drawConstRingMemoryPlot()
{
  struct GetPlotData : PlotData
  {
    GetPlotData(const PlotData &base) : PlotData{base} {}
    ImPlotPoint getAllocatedPoint(int idx)
    {
      const auto &frame = getFrame(idx);
      return {double(frame.frameIndex), double(frame.netCounters.constRing.size)};
    }

    ImPlotPoint getUsedPoint(int idx)
    {
      const auto &frame = getFrame(idx);
      return {double(frame.frameIndex), double(frame.rawCounters.usedConstRing.size)};
    }

    uint64_t getSelectedAllocate() const { return getSelectedFrame().netCounters.constRing.size; }

    uint64_t getSelectedUsed() const { return getSelectedFrame().rawCounters.usedConstRing.size; }
  };

  auto metricsAccess = accessMetrics();
  const uint64_t peakValue = metricsAccess->getLastFrameMetrics().netCountersPeak.constRing.size;

  auto createGraphViewControls = [this]() { drawGraphViewControls(Graph::CONST_RING); };

  auto selectPlotData = [this, peakValue, &metricsAccess]() //
  {
    PlotDataBase<GetPlotData> result //
      {setupPlotXRange(metricsAccess, getGraphDisplayInfo(Graph::CONST_RING), peakValue > 0)};
    setupPlotYMemoryRange(peakValue, result.hasAnyData());
    return result;
  };

  draw_ring_buffer_plot("DX12-Const-Ring-Usage-Plot-Segment", "Usage", "Usage##DX12-Const-Ring-Usage-Plot", createGraphViewControls,
    selectPlotData);
}

void MetricsVisualizer::drawConstRingBasicMetricsTable()
{
  auto metricsAccess = accessMetrics();
  const auto &frame = metricsAccess->getLastFrameMetrics();
  const auto &currentCounter = frame.rawCounters.usedConstRing;
  const auto &summaryCounter = frame.rawCountersSummary.usedConstRing;
  const auto &peakCounter = frame.rawCountersPeak.usedConstRing;
  const auto &peakCounterBuffer = frame.netCountersPeak.constRing;
  drawRingMemoryBasicMetricsTable("DX12-Temp-Const-Ring-Basic-Metrics-Table", currentCounter, summaryCounter, peakCounter,
    peakCounterBuffer);
}

void MetricsVisualizer::drawUploadRingMemoryPlot()
{
  struct GetPlotData : PlotData
  {
    GetPlotData(const PlotData &base) : PlotData{base} {}
    ImPlotPoint getAllocatedPoint(int idx)
    {
      const auto &frame = getFrame(idx);
      return {double(frame.frameIndex), double(frame.netCounters.uploadRing.size)};
    }

    ImPlotPoint getUsedPoint(int idx)
    {
      const auto &frame = getFrame(idx);
      return {double(frame.frameIndex), double(frame.rawCounters.usedUploadRing.size)};
    }

    uint64_t getSelectedAllocate() const { return getSelectedFrame().netCounters.uploadRing.size; }

    uint64_t getSelectedUsed() const { return getSelectedFrame().rawCounters.usedUploadRing.size; }
  };

  auto metricsAccess = accessMetrics();
  const uint64_t peakValue = metricsAccess->getLastFrameMetrics().netCountersPeak.uploadRing.size;

  auto createGraphViewControls = [this]() { drawGraphViewControls(Graph::UPLOAD_RING); };

  auto selectPlotData = [this, peakValue, &metricsAccess]() //
  {
    PlotDataBase<GetPlotData> result //
      {setupPlotXRange(metricsAccess, getGraphDisplayInfo(Graph::UPLOAD_RING), peakValue > 0)};
    setupPlotYMemoryRange(peakValue, result.hasAnyData());
    return result;
  };

  draw_ring_buffer_plot("DX12-Upload-Ring-Usage-Plot-Segment", "Usage", "Usage##DX12-Upload-Ring-Usage-Plot", createGraphViewControls,
    selectPlotData);
}

void MetricsVisualizer::drawUploadRingBasicMetricsTable()
{
  auto metricsAccess = accessMetrics();
  const auto &frame = metricsAccess->getLastFrameMetrics();
  const auto &currentCounter = frame.rawCounters.usedUploadRing;
  const auto &summaryCounter = frame.rawCountersSummary.usedUploadRing;
  const auto &peakCounter = frame.rawCountersPeak.usedUploadRing;
  const auto &peakCounterBuffer = frame.netCountersPeak.uploadRing;
  drawRingMemoryBasicMetricsTable("DX12-Temp-Upload-Ring-Basic-Metrics-Table", currentCounter, summaryCounter, peakCounter,
    peakCounterBuffer);
}

void MetricsVisualizer::drawTempuraryUploadMemoryPlot()
{
  struct GetPlotData : PlotData
  {
    GetPlotData(const PlotData &base) : PlotData{base} {}
    ImPlotPoint getAllocatedPoint(int idx)
    {
      const auto &frame = getFrame(idx);
      return {double(frame.frameIndex), double(frame.netCounters.tempBuffer.size)};
    }

    ImPlotPoint getUsedPoint(int idx)
    {
      const auto &frame = getFrame(idx);
      return {double(frame.frameIndex), double(frame.rawCounters.usedTempBuffer.size)};
    }

    uint64_t getSelectedAllocate() const { return getSelectedFrame().netCounters.tempBuffer.size; }

    uint64_t getSelectedUsed() const { return getSelectedFrame().rawCounters.usedTempBuffer.size; }
  };

  auto metricsAccess = accessMetrics();
  const uint64_t peakValue = metricsAccess->getLastFrameMetrics().netCountersPeak.tempBuffer.size;

  auto createGraphViewControls = [this]() { drawGraphViewControls(Graph::TEMP_MEMORY); };

  auto selectPlotData = [this, peakValue, &metricsAccess]() //
  {
    PlotDataBase<GetPlotData> result //
      {setupPlotXRange(metricsAccess, getGraphDisplayInfo(Graph::TEMP_MEMORY), peakValue > 0)};
    setupPlotYMemoryRange(peakValue, result.hasAnyData());
    return result;
  };

  draw_ring_buffer_plot("DX12-Temp-Buffer-Usage-Plot-Segment", "Usage", "Usage##DX12-Temp-Buffer-Usage-Plot", createGraphViewControls,
    selectPlotData);
}

void MetricsVisualizer::drawTemporaryUploadMemoryBasicMetrics()
{
  auto metricsAccess = accessMetrics();
  const auto &frame = metricsAccess->getLastFrameMetrics();
  const auto &currentCounter = frame.rawCounters.usedTempBuffer;
  const auto &summaryCounter = frame.rawCountersSummary.usedTempBuffer;
  const auto &peakCounter = frame.rawCountersPeak.usedTempBuffer;
  const auto &peakBufferCounter = frame.netCountersPeak.tempBuffer;
  drawRingMemoryBasicMetricsTable("DX12-Temp-Upload-Memory-Basic-Metrics-Table", currentCounter, summaryCounter, peakCounter,
    peakBufferCounter);
}

void MetricsVisualizer::drawRaytracePlot()
{
#if D3D_HAS_RAY_TRACING
  const int child_height = 17 * ImGui::GetFrameHeightWithSpacing();
  if (begin_sub_section("DX12-raytracing-plot", "Usage", child_height))
  {
    auto metricsAccess = accessMetrics();
    const auto &frame = metricsAccess->getLastFrameMetrics();
    uint64_t maxY2 = frame.netCountersPeak.raytraceTopStructure.count;
    maxY2 = max(maxY2, frame.netCountersPeak.raytraceBottomStructure.count);

    if (ImPlot::BeginPlot("Usage##DX12-raytracing-plot", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend | ImPlotFlags_YAxis2,
          ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
      auto plotData = setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::RAYTRACING), maxY2 > 0);
      uint64_t maxY = frame.netCountersPeak.raytraceTopStructure.size;
      maxY = max(maxY, frame.netCountersPeak.raytraceBottomStructure.size);

      setupPlotYMemoryRange(maxY, plotData.hasAnyData());
      setupPlotY2CountRange(maxY2, plotData.hasAnyData());

      if (plotData.hasAnyData())
      {
        struct GetHeapSize
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.raytraceAccelStructHeap.size); }
        };
        struct GetHeapCount
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.raytraceAccelStructHeap.count); }
        };
        struct GetTopSize
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.raytraceTopStructure.size); }
        };
        struct GetTopCount
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.raytraceTopStructure.count); }
        };
        struct GetBottomSize
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.raytraceBottomStructure.size); }
        };
        struct GetBottomCount
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.raytraceBottomStructure.count); }
        };
        {
          ScopedPlotStyleVar shadedAlpha{ImPlotStyleVar_FillAlpha, 0.25f};

          plot_shaded("Acceleration Structure Heap Memory (top + bottom + reserved)", getPlotPointFrameValue<ImPlotPoint, GetHeapSize>,
            getPlotPointFrameBase<ImPlotPoint>, plotData);

          plot_shaded("Top Structure Memory", getPlotPointFrameValue<ImPlotPoint, GetTopSize>, getPlotPointFrameBase<ImPlotPoint>,
            plotData);

          plot_shaded("Bottom Structure Memory", getPlotPointFrameValue<ImPlotPoint, GetBottomSize>,
            getPlotPointFrameBase<ImPlotPoint>, plotData);
        }

        plot_line("Acceleration Structure Heap Memory (top + bottom + reserved)", getPlotPointFrameValue<ImPlotPoint, GetHeapSize>,
          plotData);
        plot_line("Top Structure Memory", getPlotPointFrameValue<ImPlotPoint, GetTopSize>, plotData);
        plot_line("Bottom Structure Memory", getPlotPointFrameValue<ImPlotPoint, GetBottomSize>, plotData);

        ImPlot::SetAxis(ImAxis_Y2);

        plot_line("Acceleartion Structure Heap Count", getPlotPointFrameValue<ImPlotPoint, GetHeapCount>, plotData);
        plot_line("Top Structure Count", getPlotPointFrameValue<ImPlotPoint, GetTopCount>, plotData);
        plot_line("Bottom Structure Count", getPlotPointFrameValue<ImPlotPoint, GetBottomCount>, plotData);

        // When hovering the plot we show a tooltip with frame index and the sizes of each plot
        if (ImPlot::IsPlotHovered())
        {
          // those are actually the values in the graph where the mouse is pointing at
          auto pos = ImPlot::GetPlotMousePos();
          if (pos.x >= 0)
          {
            auto frameIdx = static_cast<uint64_t>(round(pos.x));
            if (plotData.selectFrame(frameIdx))
            {
              const auto &frame = plotData.getSelectedFrame();

              ScopedTooltip tooltip;
              ImGui::Text("Frame: %I64u", frameIdx);

              ByteUnits topSize = frame.netCounters.raytraceTopStructure.size;
              ImPlot::ItemIcon(ImPlot::GetColormapColor(0));
              ImGui::SameLine();
              ImGui::Text("Top Structure Memory: %.2f %s", topSize.units(), topSize.name());

              ByteUnits bottomSize = frame.netCounters.raytraceBottomStructure.size;
              ImPlot::ItemIcon(ImPlot::GetColormapColor(1));
              ImGui::SameLine();
              ImGui::Text("Bottom Structure Memory: %.2f %s", bottomSize.units(), bottomSize.name());

              ByteUnits heapSize = frame.netCounters.raytraceAccelStructHeap.size;
              ImPlot::ItemIcon(ImPlot::GetColormapColor(2));
              ImGui::SameLine();
              ImGui::Text("Accelearation Structure Heap Memory: %.2f %s", heapSize.units(), heapSize.name());

              ImPlot::ItemIcon(ImPlot::GetColormapColor(3));
              ImGui::SameLine();
              ImGui::Text("Top Structure Count: %I64u", frame.netCounters.raytraceTopStructure.count);

              ImPlot::ItemIcon(ImPlot::GetColormapColor(4));
              ImGui::SameLine();
              ImGui::Text("Bottom Structure Count: %I64u", frame.netCounters.raytraceBottomStructure.count);

              ImPlot::ItemIcon(ImPlot::GetColormapColor(5));
              ImGui::SameLine();
              ImGui::Text("Accelearation Structure Heap Count: %I64u", frame.netCounters.raytraceAccelStructHeap.count);
            }
          }
        }
      }
      else
      {
        // When no data is available just display a empty plot
        ImPlot::PlotDummy("Acceleration Structure Heap Memory (top + bottom + reserved)");
        ImPlot::PlotDummy("Top Structure Memory");
        ImPlot::PlotDummy("Bottom Structure Memory");
        ImPlot::PlotDummy("Acceleartion Structure Heap Count");
        ImPlot::PlotDummy("Top Structure Count");
        ImPlot::PlotDummy("Bottom Structure Count");
        // Tell the user, there nothing to show in the plot
        ImPlot::PushStyleColor(ImPlotCol_InlayText, ImVec4(.75f, .75f, 0.f, 1.f));
        ImPlot::PlotText("No historical data available", 0.5, 0.5);
        ImPlot::PopStyleColor();
      }
      ImPlot::EndPlot();
    }
  }
  end_sub_section();
#endif
}

void MetricsVisualizer::drawUserHeapsPlot()
{
  const int child_height = 17 * ImGui::GetFrameHeightWithSpacing();
  if (begin_sub_section("DX12-Heap-Graph-Segment", "Usage", child_height))
  {
    auto metricsAccess = accessMetrics();
    const auto &frame = metricsAccess->getLastFrameMetrics();
    uint64_t maxY2 = frame.netCountersPeak.userResourceHeaps.count;

    if (ImPlot::BeginPlot("Usage##DX12-User-Heap-Graph", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend | ImPlotFlags_YAxis2,
          ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {

      auto plotData = setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::USER_HEAPS), maxY2 > 0);
      // heap size will always be the largest
      setupPlotYMemoryRange(frame.netCountersPeak.userResourceHeaps.size, plotData.hasAnyData());
      setupPlotY2CountRange(maxY2, plotData.hasAnyData());

      if (plotData.hasAnyData())
      {
        struct GetUserHeapSize
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.userResourceHeaps.size); }
        };
        struct GetUserHeapCount
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.userResourceHeaps.count); }
        };
        {
          ScopedPlotStyleVar shadedAlpha{ImPlotStyleVar_FillAlpha, 0.25f};
          plot_shaded("User Heaps Size", getPlotPointFrameValue<ImPlotPoint, GetUserHeapSize>, getPlotPointFrameBase<ImPlotPoint>,
            plotData);
        }

        plot_line("User Heaps Size", getPlotPointFrameValue<ImPlotPoint, GetUserHeapSize>, plotData);

        ImPlot::SetAxis(ImAxis_Y2);

        plot_line("User Heap Count", getPlotPointFrameValue<ImPlotPoint, GetUserHeapCount>, plotData);

        // When hovering the plot we show a tooltip with frame index and the sizes of each plot
        if (ImPlot::IsPlotHovered())
        {
          // those are actually the values in the graph where the mouse is pointing at
          auto pos = ImPlot::GetPlotMousePos();
          if (pos.x >= 0)
          {
            auto frameIdx = static_cast<uint64_t>(round(pos.x));
            if (plotData.selectFrame(frameIdx))
            {
              const auto &frame = plotData.getSelectedFrame();

              ScopedTooltip tooltip;
              ImGui::Text("Frame: %I64u", frameIdx);

              auto userHeapSizeUnit = size_to_unit_table(frame.netCounters.userResourceHeaps.size);
              ImPlot::ItemIcon(ImPlot::GetColormapColor(0));
              ImGui::SameLine();
              ImGui::Text("User Heaps Size: %.2f %s",
                compute_unit_type_size(frame.netCounters.userResourceHeaps.size, userHeapSizeUnit), get_unit_name(userHeapSizeUnit));

              ImPlot::ItemIcon(ImPlot::GetColormapColor(1));
              ImGui::SameLine();
              ImGui::Text("USer Heap Count: %I64u", frame.netCounters.userResourceHeaps.count);
            }
          }
        }
      }
      else
      {
        // When no data is available just display a empty plot
        ImPlot::PlotDummy("User Heaps Size");
        ImPlot::PlotDummy("User Heap Count");
        // Tell the user, there nothing to show in the plot
        ImPlot::PushStyleColor(ImPlotCol_InlayText, ImVec4(.75f, .75f, 0.f, 1.f));
        ImPlot::PlotText("No historical data available", 0.5, 0.5);
        ImPlot::PopStyleColor();
      }
      ImPlot::EndPlot();
    }
  }
  end_sub_section();
}

void MetricsVisualizer::drawPersistenUploadMemoryMetricsTable()
{
  auto metricsAccess = accessMetrics();
  const auto &frame = metricsAccess->getLastFrameMetrics();

  drawPersistentBufferBasicMetricsTable("DX12-Persistent-Upload-Basic-Metrics-Table", frame.netCounters.persistentUploadMemory,
    frame.netCounters.persistentUploadBuffer, frame.netCountersPeak.persistentUploadMemory,
    frame.netCountersPeak.persistentUploadBuffer);
}

void MetricsVisualizer::drawPersistenReadBackMemoryMetricsTable()
{
  auto metricsAccess = accessMetrics();
  const auto &frame = metricsAccess->getLastFrameMetrics();

  drawPersistentBufferBasicMetricsTable("DX12-Persistent-Read-Back-Basic-Metrics-Table", frame.netCounters.persistentReadBackMemory,
    frame.netCounters.persistentReadBackBuffer, frame.netCountersPeak.persistentReadBackMemory,
    frame.netCountersPeak.persistentReadBackBuffer);
}

void MetricsVisualizer::drawPersistenBidirectioanlMemoryMetricsTable()
{
  auto metricsAccess = accessMetrics();
  const auto &frame = metricsAccess->getLastFrameMetrics();

  drawPersistentBufferBasicMetricsTable("DX12-Persistent-Bidirectional-Basic-Metrics-Table",
    frame.netCounters.persistentBidirectionalMemory, frame.netCounters.persistentBidirectionalBuffer,
    frame.netCountersPeak.persistentBidirectionalMemory, frame.netCountersPeak.persistentBidirectionalBuffer);
}

void MetricsVisualizer::drawPersistenUploadMemoryPlot()
{
  struct GetPlotData : PlotData
  {
    GetPlotData(const PlotData &base) : PlotData{base} {}
    ImPlotPoint getAllocatedPoint(int idx)
    {
      const auto &frame = getFrame(idx);
      return {double(frame.frameIndex), double(frame.netCounters.persistentUploadBuffer.size)};
    }

    ImPlotPoint getUsedPoint(int idx)
    {
      const auto &frame = getFrame(idx);
      return {double(frame.frameIndex), double(frame.netCounters.persistentUploadMemory.size)};
    }

    uint64_t getSelectedAllocate() const { return getSelectedFrame().netCounters.persistentUploadBuffer.size; }

    uint64_t getSelectedUsed() const { return getSelectedFrame().netCounters.persistentUploadMemory.size; }
  };

  auto metricsAccess = accessMetrics();
  const uint64_t peakValue = metricsAccess->getLastFrameMetrics().netCountersPeak.persistentUploadBuffer.size;

  auto createGraphViewControls = [this]() { drawGraphViewControls(Graph::UPLOAD_MEMORY); };

  auto selectPlotData = [this, peakValue, &metricsAccess]() //
  {
    PlotDataBase<GetPlotData> result //
      {setupPlotXRange(metricsAccess, getGraphDisplayInfo(Graph::UPLOAD_MEMORY), peakValue > 0)};
    setupPlotYMemoryRange(peakValue, result.hasAnyData());
    return result;
  };

  draw_ring_buffer_plot("DX12Persistent-Upload-Memory-Usage-Plot-Segment", "Usage", "Usage##DX12Persistent-Upload-Memory-Usage-Plot",
    createGraphViewControls, selectPlotData);
}

void MetricsVisualizer::drawPersistenReadBackMemoryPlot()
{
  struct GetPlotData : PlotData
  {
    GetPlotData(const PlotData &base) : PlotData{base} {}
    ImPlotPoint getAllocatedPoint(int idx)
    {
      const auto &frame = getFrame(idx);
      return {double(frame.frameIndex), double(frame.netCounters.persistentReadBackBuffer.size)};
    }

    ImPlotPoint getUsedPoint(int idx)
    {
      const auto &frame = getFrame(idx);
      return {double(frame.frameIndex), double(frame.netCounters.persistentReadBackMemory.size)};
    }

    uint64_t getSelectedAllocate() const { return getSelectedFrame().netCounters.persistentReadBackBuffer.size; }

    uint64_t getSelectedUsed() const { return getSelectedFrame().netCounters.persistentReadBackMemory.size; }
  };

  auto metricsAccess = accessMetrics();
  const uint64_t peakValue = metricsAccess->getLastFrameMetrics().netCountersPeak.persistentReadBackBuffer.size;

  auto createGraphViewControls = [this]() { drawGraphViewControls(Graph::READ_BACK_MEMORY); };

  auto selectPlotData = [this, peakValue, &metricsAccess]() //
  {
    PlotDataBase<GetPlotData> result //
      {setupPlotXRange(metricsAccess, getGraphDisplayInfo(Graph::READ_BACK_MEMORY), peakValue > 0)};
    setupPlotYMemoryRange(peakValue, result.hasAnyData());
    return result;
  };

  draw_ring_buffer_plot("DX12Persistent-Read-Back-Memory-Usage-Plot-Segment", "Usage",
    "Usage##DX12Persistent-Read-Back-Memory-Usage-Plot", createGraphViewControls, selectPlotData);
}

void MetricsVisualizer::drawPersistenBidirectioanlMemoryPlot()
{
  struct GetPlotData : PlotData
  {
    GetPlotData(const PlotData &base) : PlotData{base} {}
    ImPlotPoint getAllocatedPoint(int idx)
    {
      const auto &frame = getFrame(idx);
      return {double(frame.frameIndex), double(frame.netCounters.persistentBidirectionalBuffer.size)};
    }

    ImPlotPoint getUsedPoint(int idx)
    {
      const auto &frame = getFrame(idx);
      return {double(frame.frameIndex), double(frame.netCounters.persistentBidirectionalMemory.size)};
    }

    uint64_t getSelectedAllocate() const { return getSelectedFrame().netCounters.persistentBidirectionalBuffer.size; }

    uint64_t getSelectedUsed() const { return getSelectedFrame().netCounters.persistentBidirectionalMemory.size; }
  };

  auto metricsAccess = accessMetrics();
  const uint64_t peakValue = metricsAccess->getLastFrameMetrics().netCountersPeak.persistentBidirectionalBuffer.size;

  auto createGraphViewControls = [this]() { drawGraphViewControls(Graph::BIDIRECTIONAL_MEMORY); };

  auto selectPlotData = [this, peakValue, &metricsAccess]() //
  {
    PlotDataBase<GetPlotData> result //
      {setupPlotXRange(metricsAccess, getGraphDisplayInfo(Graph::BIDIRECTIONAL_MEMORY), peakValue > 0)};
    setupPlotYMemoryRange(peakValue, result.hasAnyData());
    return result;
  };

  draw_ring_buffer_plot("DX12Persistent-Bidirectional-Memory-Usage-Plot-Segment", "Usage",
    "Usage##DX12Persistent-Bidirectional-Memory-Usage-Plot", createGraphViewControls, selectPlotData);
}

void MetricsVisualizer::drawRaytraceSummaryTable()
{
#if D3D_HAS_RAY_TRACING
  if (
    !ImGui::BeginTable("DX12-raytrace-summary-table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    return;
  }

  make_table_header({"Property", "Value", "Peak"});

  auto metricsAccess = accessMetrics();
  const auto &frameCounters = metricsAccess->getLastFrameMetrics();

  begin_selectable_row("Acceleration structure heap memory size");
  ByteUnits heapSize = frameCounters.netCounters.raytraceAccelStructHeap.size;
  ImGui::TableNextColumn();
  ImGui::Text("%.2f %s", heapSize.units(), heapSize.name());
  heapSize = frameCounters.netCountersPeak.raytraceAccelStructHeap.size;
  ImGui::TableNextColumn();
  ImGui::Text("%.2f %s", heapSize.units(), heapSize.name());

  begin_selectable_row("Acceleration structure heap count");
  ImGui::TableNextColumn();
  ImGui::Text("%I64u", frameCounters.netCounters.raytraceAccelStructHeap.count);
  ImGui::TableNextColumn();
  ImGui::Text("%I64u", frameCounters.netCountersPeak.raytraceAccelStructHeap.count);

  begin_selectable_row("Top structure memory size");
  ByteUnits topSize = frameCounters.netCounters.raytraceTopStructure.size;
  ImGui::TableNextColumn();
  ImGui::Text("%.2f %s", topSize.units(), topSize.name());
  topSize = frameCounters.netCountersPeak.raytraceTopStructure.size;
  ImGui::TableNextColumn();
  ImGui::Text("%.2f %s", topSize.units(), topSize.name());

  begin_selectable_row("Top structure memory count");
  ImGui::TableNextColumn();
  ImGui::Text("%I64u", frameCounters.netCounters.raytraceTopStructure.count);
  ImGui::TableNextColumn();
  ImGui::Text("%I64u", frameCounters.netCountersPeak.raytraceTopStructure.count);

  begin_selectable_row("Bottom structure memory size");
  ByteUnits bottomSize = frameCounters.netCounters.raytraceBottomStructure.size;
  ImGui::TableNextColumn();
  ImGui::Text("%.2f %s", bottomSize.units(), bottomSize.name());
  bottomSize = frameCounters.netCountersPeak.raytraceBottomStructure.size;
  ImGui::TableNextColumn();
  ImGui::Text("%.2f %s", bottomSize.units(), bottomSize.name());

  begin_selectable_row("Bottom structure memory count");
  ImGui::TableNextColumn();
  ImGui::Text("%I64u", frameCounters.netCounters.raytraceBottomStructure.count);
  ImGui::TableNextColumn();
  ImGui::Text("%I64u", frameCounters.netCountersPeak.raytraceBottomStructure.count);

  ImGui::EndTable();
#endif
}
#else
void MetricsVisualizer::drawMetricsCaptureControls()
{
  ScopedWarningTextStyle warningStyle;
  ImGui::TextUnformatted("No controls available as metrics are not part of this build");
}

void MetricsVisualizer::drawMetricsEventsView()
{
  ScopedWarningTextStyle warningStyle;
  ImGui::TextUnformatted("No events available as metrics are not part of this build");
}

void MetricsVisualizer::drawMetricsEvnetsViewFilterControls() {}

void MetricsVisualizer::drawSystemCurrentMemoryUseTable()
{
  ScopedWarningTextStyle warningStyle;
  // TODO: we could use current values to show live values
  ImGui::TextUnformatted("No system memory usage statistics as metrics are not part of this build");
}

void MetricsVisualizer::drawSystemMemoryUsePlot() {}

void MetricsVisualizer::drawHeapsPlot() {}

void MetricsVisualizer::drawHeapsSummaryTable() {}

void MetricsVisualizer::drawScratchBufferTable() {}

void MetricsVisualizer::drawScratchBufferPlot() {}

void MetricsVisualizer::drawTexturePlot() {}

void MetricsVisualizer::drawBuffersPlot() {}

void MetricsVisualizer::drawConstRingMemoryPlot() {}

void MetricsVisualizer::drawConstRingBasicMetricsTable() {}

void MetricsVisualizer::drawUploadRingMemoryPlot() {}

void MetricsVisualizer::drawUploadRingBasicMetricsTable() {}

void MetricsVisualizer::drawTempuraryUploadMemoryPlot() {}

void MetricsVisualizer::drawTemporaryUploadMemoryBasicMetrics() {}

void MetricsVisualizer::drawRaytracePlot() {}

void MetricsVisualizer::drawUserHeapsPlot() {}

void MetricsVisualizer::drawPersistenUploadMemoryMetricsTable() {}

void MetricsVisualizer::drawPersistenReadBackMemoryMetricsTable() {}

void MetricsVisualizer::drawPersistenBidirectioanlMemoryMetricsTable() {}

void MetricsVisualizer::drawPersistenUploadMemoryPlot() {}

void MetricsVisualizer::drawPersistenReadBackMemoryPlot() {}

void MetricsVisualizer::drawPersistenBidirectioanlMemoryPlot() {}

void MetricsVisualizer::drawRaytraceSummaryTable() {}
#endif

void DebugView::drawPersistenUploadMemorySegmentTable()
{
  ImGui::BeginTable("DX12-persistent-upload-memory-table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
  make_table_header({"", "CPU Pointer", "GPU Pointer", "Allocated", "Size"});
  ID3D12Resource *lastBuf = nullptr;
  {
    auto uploadMemoryAccess = uploadMemory.access();
    for (;;)
    {
      decltype(&uploadMemoryAccess->buffers.front()) candidate = nullptr;
      for (auto &buffer : uploadMemoryAccess->buffers)
      {
        if (buffer.buffer.Get() <= lastBuf)
        {
          continue;
        }
        if (candidate && candidate->buffer.Get() < buffer.buffer.Get())
        {
          continue;
        }
        candidate = &buffer;
      }

      if (!candidate)
      {
        break;
      }
      lastBuf = candidate->buffer.Get();
      draw_persisten_buffer_table_rows(*candidate);
    }
  }
  ImGui::EndTable();
}

void DebugView::drawPersistenReadBackMemorySegmentTable()
{
  ImGui::BeginTable("DX12-persistent-read-back-memory-table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
  make_table_header({"", "CPU Pointer", "GPU Pointer", "Allocated", "Size"});
  ID3D12Resource *lastBuf = nullptr;
  {
    auto readBackMemoryAccess = readBackMemory.access();
    for (;;)
    {
      decltype(&readBackMemoryAccess->buffers.front()) candidate = nullptr;
      for (auto &buffer : readBackMemoryAccess->buffers)
      {
        if (buffer.buffer.Get() <= lastBuf)
        {
          continue;
        }
        if (candidate && candidate->buffer.Get() < buffer.buffer.Get())
        {
          continue;
        }
        candidate = &buffer;
      }

      if (!candidate)
      {
        break;
      }
      lastBuf = candidate->buffer.Get();
      draw_persisten_buffer_table_rows(*candidate);
    }
  }
  ImGui::EndTable();
}

void DebugView::drawPersistenBidirectioanlMemorySegmentTable()
{
  ImGui::BeginTable("DX12-persistent-bidirectional-memory-table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
  make_table_header({"", "CPU Pointer", "GPU Pointer", "Allocated", "Size"});
  ID3D12Resource *lastBuf = nullptr;
  {
    auto bidirectionalMemoryAccess = bidirectionalMemory.access();
    for (;;)
    {
      decltype(&bidirectionalMemoryAccess->buffers.front()) candidate = nullptr;
      for (auto &buffer : bidirectionalMemoryAccess->buffers)
      {
        if (buffer.buffer.Get() <= lastBuf)
        {
          continue;
        }
        if (candidate && candidate->buffer.Get() < buffer.buffer.Get())
        {
          continue;
        }
        candidate = &buffer;
      }

      if (!candidate)
      {
        break;
      }
      lastBuf = candidate->buffer.Get();
      draw_persisten_buffer_table_rows(*candidate);
    }
  }
  ImGui::EndTable();
}

void DebugView::drawPersistenUploadMemoryInfoView()
{
  drawPersistenUploadMemoryMetricsTable();

  drawPersistenUploadMemoryPlot();

  drawPersistenUploadMemorySegmentTable();
}

void DebugView::drawPersistenReadBackMemoryInfoView()
{
  drawPersistenReadBackMemoryMetricsTable();

  drawPersistenReadBackMemoryPlot();

  drawPersistenReadBackMemorySegmentTable();
}

void DebugView::drawPersistenBidirectioanlMemoryInfoView()
{
  drawPersistenBidirectioanlMemoryMetricsTable();

  drawPersistenBidirectioanlMemoryPlot();

  drawPersistenBidirectioanlMemorySegmentTable();
}

void DebugView::drawUserHeapsTable()
{
  constexpr int table_height = 15;
  if (ImGui::BeginTable("DX12-Heap-Table", 5,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
        ImVec2(0, table_height * ImGui::GetFrameHeightWithSpacing())))
  {
    make_table_header( //
      {"Identifier", "Children", "Placement Start", "Placement End", "Total Size"});
    {
      auto aliasHeapsAccess = aliasHeaps.access();
      for (auto &&heap : *aliasHeapsAccess)
      {
        if (!heap)
        {
          continue;
        }
        auto totalSizeUnit = size_to_unit_table(heap.memory.size());
        ResourceHeapProperties properties;
        properties.raw = heap.memory.getHeapID().group;

#if _TARGET_XBOX
        auto heapPointer = heap.memory.asPointer();
#if _TARGET_SCARLETT
        const char *heapTypePropertyName = properties.isGPUOptimal ? "GPU Optimal Bandwidth" : "Standard Bandwidth";
#else
        const char *heapTypePropertyName = "Standard Bandwidth";
#endif
        const char *cpuCachePropertyName = properties.isCPUCoherent ? "CPU Coherent" : "CPU Write Combine";
        const char *miscHeapPropertyName = properties.isGPUExecutable ? "GPU Executable Read Write" : "GPU Read Write";
        uint32_t heapFlags = properties.raw;
#else
        auto heapPointer = heap.memory.getHeap();
        const char *heapTypePropertyName = to_string(properties.getMemoryPool(isUMASystem()));
        const char *cpuCachePropertyName = to_string(properties.getCpuPageProperty(isUMASystem()));
        const char *miscHeapPropertyName = to_string(properties.getHeapType());
        uint32_t heapFlags = static_cast<uint32_t>(properties.getFlags(canMixResources()));
#endif

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        char strBuf[256];
        sprintf_s(strBuf, "%p 0x%08X <%s> <%s> <%s>", heapPointer, heapFlags, heapTypePropertyName, cpuCachePropertyName,
          miscHeapPropertyName);
        bool open = ImGui::TreeNodeEx(strBuf, ImGuiTreeNodeFlags_SpanFullWidth);

        uint32_t children = heap.images.size() + heap.buffers.size();
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u", children);

        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%.2f %s", compute_unit_type_size(heap.memory.size(), totalSizeUnit), get_unit_name(totalSizeUnit));
        if (open)
        {
          for (auto image : heap.images)
          {
            image->getDebugName([](auto &name) { begin_selectable_row(name); });

            auto mem = image->getMemory();
            auto offset = heap.memory.calculateOffset(mem);
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%016llX", offset);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%016llX", offset + mem.size());
            ImGui::TableSetColumnIndex(4);
            auto sizeUnits = size_to_unit_table(mem.size());
            ImGui::Text("%.f %s", compute_unit_type_size(mem.size(), sizeUnits), get_unit_name(sizeUnits));
          }
          {
            auto bufferHeapStateAccess = bufferHeapState.access();
            for (auto bufferID : heap.buffers)
            {
              auto &buffer = bufferHeapStateAccess->bufferHeaps[bufferID.index()];

              char resnameBuffer[MAX_OBJECT_NAME_LENGTH];
              begin_selectable_row(get_resource_name(buffer.buffer.Get(), resnameBuffer));

              auto &mem = buffer.bufferMemory;
              auto offset = heap.memory.calculateOffset(mem);
              ImGui::TableSetColumnIndex(2);
              ImGui::Text("%016llX", offset);
              ImGui::TableSetColumnIndex(3);
              ImGui::Text("%016llX", offset + mem.size());
              ImGui::TableSetColumnIndex(4);
              auto sizeUnits = size_to_unit_table(mem.size());
              ImGui::Text("%.f %s", compute_unit_type_size(mem.size(), sizeUnits), get_unit_name(sizeUnits));
            }
          }
          ImGui::TreePop();
        }
      }
    }
    ImGui::EndTable();
  }
}

void DebugView::drawUserHeapsSummaryTable()
{
  if (
    ImGui::BeginTable("DX12-User-Heap-Summary-Table", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    make_table_header( //
      {"Heaps", "Heaps Size", "Textures", "Texture Size", "Buffers", "Buffer Size", "Total Size"});
    char buf[64];
    auto aliasHeapsAccess = aliasHeaps.access();
    sprintf_s(buf, "%I64u", uint64_t(aliasHeapsAccess->size()));
    begin_selectable_row(buf);

    size_t heapsSize = 0;
    size_t textureSize = 0;
    size_t textureCount = 0;
    size_t bufferSize = 0;
    size_t bufferCount = 0;
    {
      auto bufferHeapStateAccess = bufferHeapState.access();
      for (auto &heap : *aliasHeapsAccess)
      {
        if (!heap)
        {
          continue;
        }
        heapsSize += heap.memory.size();
        textureCount += heap.images.size();
        bufferCount += heap.buffers.size();

        for (auto tex : heap.images)
        {
          textureSize += tex->getMemory().size();
        }

        for (auto bufferID : heap.buffers)
        {
          auto &buffer = bufferHeapStateAccess->bufferHeaps[bufferID.index()];
          bufferSize += buffer.bufferMemory.size();
        }
      }
    }
    auto heapSizeUnits = size_to_unit_table(heapsSize);
    auto textureSizeUnits = size_to_unit_table(textureSize);
    auto bufferSizeUnits = size_to_unit_table(bufferSize);
    auto totalSizeUnits = size_to_unit_table(textureSize + bufferSize);

    ImGui::TableNextColumn();
    ImGui::Text("%2.f %s", compute_unit_type_size(heapsSize, heapSizeUnits), get_unit_name(heapSizeUnits));

    ImGui::TableNextColumn();
    ImGui::Text("%I64u", uint64_t(textureCount));

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s", compute_unit_type_size(textureSize, textureSizeUnits), get_unit_name(textureSizeUnits));

    ImGui::TableNextColumn();
    ImGui::Text("%I64u", uint64_t(bufferCount));

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s", compute_unit_type_size(bufferSize, bufferSizeUnits), get_unit_name(bufferSizeUnits));

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s", compute_unit_type_size(textureSize + bufferSize, totalSizeUnits), get_unit_name(totalSizeUnits));

    ImGui::EndTable();
  }
}

void DebugView::drawRaytraceTopStructureTable()
{
#if D3D_HAS_RAY_TRACING
  if (!ImGui::TreeNodeEx("Top Acceleration Structures##DX12-top-structure-table-tree", ImGuiTreeNodeFlags_Framed))
  {
    return;
  }

  // address from, address to, size
  if (ImGui::BeginTable("DX12-top-structure-table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    make_table_header({"Address start", "Address end", "Size"});

    {
      OSSpinlockScopedLock lock{rtasSpinlock};
      accelStructPool.iterateAllocated([](RaytraceAccelerationStructure *as) //
        {
          // currently the only indicator for top or bottom is to see if it has a UAV descriptor or
          // not if not then its bottom
          if (0 == as->descriptor.ptr)
          {
            return;
          }

          char buf[64];
          sprintf_s(buf, "%016I64X", as->gpuAddress);
          begin_selectable_row(buf);

          ImGui::TableNextColumn();
          ImGui::Text("%016I64X", as->gpuAddress + as->size);

          ByteUnits sz = as->size;
          ImGui::TableNextColumn();
          ImGui::Text("%.2f %s", sz.units(), sz.name());
        });
    }

    ImGui::EndTable();
  }
  ImGui::TreePop();
#endif
}

void DebugView::drawRaytraceBottomStructureTable()
{
#if D3D_HAS_RAY_TRACING
  if (!ImGui::TreeNodeEx("Bottom Acceleration Structures##DX12-bottom-structure-table-tree", ImGuiTreeNodeFlags_Framed))
  {
    return;
  }

  // address from, address to, size
  if (ImGui::BeginTable("DX12-bottom-structure-table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    make_table_header({"Address start", "Address end", "Size"});

    {
      OSSpinlockScopedLock lock{rtasSpinlock};
      accelStructPool.iterateAllocated([](RaytraceAccelerationStructure *as) //
        {
          // currently the only indicator for top or bottom is to see if it has a UAV descriptor or
          // not if not then its bottom
          if (0 != as->descriptor.ptr)
          {
            return;
          }

          char buf[64];
          sprintf_s(buf, "%016I64X", as->gpuAddress);
          begin_selectable_row(buf);

          ImGui::TableNextColumn();
          ImGui::Text("%016I64X", as->gpuAddress + as->size);

          ByteUnits sz = as->size;
          ImGui::TableNextColumn();
          ImGui::Text("%.2f %s", sz.units(), sz.name());
        });
    }

    ImGui::EndTable();
  }
  ImGui::TreePop();
#endif
}

void DebugView::drawTempuraryUploadMemorySegmentsTable()
{
  if (ImGui::BeginTable("DX12-Temp-Buffer-Segment-Table", 6,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    make_table_header({"Buffer Object", "CPU Pointer", "GPU Pointer", "Size", "Allocations", "Allocated"});

    ID3D12Resource *lastBuf = nullptr;
    {
      auto tempBufferAccess = tempBuffer.access();
      for (;;)
      {
        decltype(&tempBufferAccess->currentBuffer) candidate = nullptr;
        // updates candidate if the buffer has a larger address and a smaller address than the last
        // candidate
        auto updateCandidate = [&candidate, &lastBuf](auto &contender) //
        {
          if (contender.buffer.Get() <= lastBuf)
            return;
          if (candidate && candidate->buffer.Get() < contender.buffer.Get())
            return;
          candidate = &contender;
        };
        if (tempBufferAccess->currentBuffer)
        {
          updateCandidate(tempBufferAccess->currentBuffer);
        }
        if (tempBufferAccess->standbyBuffer)
        {
          updateCandidate(tempBufferAccess->standbyBuffer);
        }
        for (auto &buffer : tempBufferAccess->buffers)
        {
          updateCandidate(buffer);
        }
        if (!candidate)
        {
          break;
        }
        auto &buffer = *candidate;
        lastBuf = buffer.buffer.Get();
        auto sizeUnits = size_to_unit_table(buffer.bufferMemory.size());
        auto usageUnits = size_to_unit_table(buffer.fillSize);
        char strBuf[32];
        sprintf_s(strBuf, "%p", buffer.buffer.Get());
        begin_selectable_row(strBuf);
        ImGui::TableNextColumn();
        ImGui::Text("%p", buffer.getCPUPointer());
        ImGui::TableNextColumn();
        ImGui::Text("%016I64X", buffer.getGPUPointer());
        ImGui::TableNextColumn();
        ImGui::Text("%.2f %s", compute_unit_type_size(buffer.bufferMemory.size(), sizeUnits), get_unit_name(sizeUnits));
        ImGui::TableNextColumn();
        ImGui::Text("%u", buffer.allocations);
        ImGui::TableNextColumn();
        ImGui::Text("%.2f %s", compute_unit_type_size(buffer.fillSize, usageUnits), get_unit_name(usageUnits));
      }
    }
    ImGui::EndTable();
  }
}

void DebugView::drawUploadRingSegmentsTable()
{
  if (ImGui::BeginTable("DX12-Upload-Ring-Segment-Table", 6,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    make_table_header({"Buffer Object", "CPU Pointer", "GPU Pointer", "Size", "Ring offset", "Allocated"});

    {
      auto uploadRingAccess = uploadRing.access();
      for (auto &segment : uploadRingAccess->ringSegments)
      {
        auto sizeUnits = size_to_unit_table(segment.bufferMemory.size());
        auto allocatedUnits = size_to_unit_table(segment.allocationSize);
        char bufPtr[32];
        sprintf_s(bufPtr, "%p", segment.buffer.Get());
        begin_selectable_row(bufPtr);
        if (ImGui::IsItemHovered())
        {
          if (segment.timeSinceUnused > 0)
          {
            ScopedTooltip tooltip;
            ImGui::Text("%u frame since last use", segment.timeSinceUnused);
          }
        }
        ImGui::TableNextColumn();
        ImGui::Text("%p", segment.getCPUPointer());
        ImGui::TableNextColumn();
        ImGui::Text("%016I64X", segment.getGPUPointer());
        ImGui::TableNextColumn();
        ImGui::Text("%.2f %s", compute_unit_type_size(segment.bufferMemory.size(), sizeUnits), get_unit_name(sizeUnits));
        ImGui::TableNextColumn();
        ImGui::Text("%08X", segment.allocationOffset);
        ImGui::TableNextColumn();
        ImGui::Text("%.2f %s", compute_unit_type_size(segment.allocationSize, allocatedUnits), get_unit_name(allocatedUnits));
      }
    }
    ImGui::EndTable();
  }
}

void DebugView::drawConstRingSegmentsTable()
{
  if (
    ImGui::BeginTable("DX12-Const-Ring-Segment-Table", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    make_table_header({"Buffer Object", "CPU Pointer", "GPU Pointer", "Size", "Ring offset", "Allocated"});

    {
      auto pushRingAccess = pushRing.access();
      for (auto &segment : pushRingAccess->ringSegments)
      {
        auto sizeUnits = size_to_unit_table(segment.bufferMemory.size());
        auto allocatedUnits = size_to_unit_table(segment.allocationSize);
        char bufPtr[32];
        sprintf_s(bufPtr, "%p", segment.buffer.Get());
        begin_selectable_row(bufPtr);
        if (ImGui::IsItemHovered())
        {
          if (segment.timeSinceUnused > 0)
          {
            ScopedTooltip tooltip;
            ImGui::Text("%u frames since last used", segment.timeSinceUnused);
          }
        }
        ImGui::TableNextColumn();
        ImGui::Text("%p", segment.getCPUPointer());
        ImGui::TableNextColumn();
        ImGui::Text("%016I64X", segment.getGPUPointer());
        ImGui::TableNextColumn();
        ImGui::Text("%.2f %s", compute_unit_type_size(segment.bufferMemory.size(), sizeUnits), get_unit_name(sizeUnits));
        ImGui::TableNextColumn();
        ImGui::Text("%08X", segment.allocationOffset);
        ImGui::TableNextColumn();
        ImGui::Text("%.2f %s", compute_unit_type_size(segment.allocationSize, allocatedUnits), get_unit_name(allocatedUnits));
      }
    }
    ImGui::EndTable();
  }
}

void DebugView::drawTextureTable()
{
  // sum of all texels, not a very useful, but interesting metric
  ByteUnits texutreSize;
  uint64_t subresouceCount = 0;
  uint64_t texelSummaryCount = 0;
  uint32_t textureCount = 0;
  if (ImGui::BeginTable("DX12-Texture-Table", 7,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
  {
    make_table_header( //
      {"Name", "Subresource ID Range", "Extents", "Arrays", "MipMaps", "Format", "Size"});

    visitImageObjects([&texelSummaryCount, &textureCount, &texutreSize, &subresouceCount](auto image) {
      ++textureCount;

      image->getDebugName([](auto &name) { begin_selectable_row(name); });

      ImGui::TableSetColumnIndex(1);
      if (image->getGlobalSubresourceIdRange().empty())
      {
        ImGui::TextUnformatted("-");
      }
      else
      {
        ImGui::Text("%u - %u", image->getGlobalSubresourceIdRange().front().index(),
          image->getGlobalSubresourceIdRange().back().index());
      }

      ImGui::TableSetColumnIndex(2);
      const auto &extent = image->getBaseExtent();
      ImGui::Text("%u x %u x %u", extent.width, extent.height, extent.depth);

      ImGui::TableSetColumnIndex(3);
      const auto layerCount = image->getArrayLayers();
      ImGui::Text("%u", layerCount.count());

      ImGui::TableSetColumnIndex(4);
      const auto mipCount = image->getMipLevelRange();
      ImGui::Text("%u", mipCount.count());

      ImGui::TableSetColumnIndex(5);
      const auto format = image->getFormat();
      // skip DXGI_FORMAT_ section
      ImGui::TextUnformatted(format.getNameString() + 12);

      subresouceCount += (mipCount * layerCount * format.getPlanes()).count();

      ImGui::TableSetColumnIndex(6);
      auto mem = image->getMemory();
      if (mem)
      {
        ByteUnits size = mem.size();
        texutreSize += size;
        ImGui::Text("%.2f %s", size.units(), size.name());
      }
      else
      {
        ImGui::TextDisabled("-");
      }

      for (auto m : image->getMipLevelRange())
      {
        auto mipExtent = image->getMipExtents(m);
        auto layerTexelCount = mipExtent.width * mipExtent.height * mipExtent.depth;
        texelSummaryCount += layerTexelCount * image->getArrayLayers().count();
      }
    });
    ImGui::EndTable();
  }

  if (ImGui::BeginTable("DX12-Texture-Summary-Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    make_table_header( //
      {"Texel Count", "Texture Count", "Total Size"});
    const char *unitNames[] = {"", "K", "M", "G"};
    uint32_t texelUnitIndex = 0;
    texelUnitIndex += texelSummaryCount > 1000;
    texelUnitIndex += texelSummaryCount > 1000000;
    texelUnitIndex += texelSummaryCount > 1000000000;
    char strBuf[32];
    sprintf_s(strBuf, "%.2f %s", texelSummaryCount / powf(1000, texelUnitIndex), unitNames[texelUnitIndex]);
    begin_selectable_row(strBuf);

    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%u", textureCount);

    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.2f %s", texutreSize.units(), texutreSize.name());
    ImGui::EndTable();
  }

  if (ImGui::BeginTable("DX12-Texture-Subresource-Summary-Table", 4,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    make_table_header( //
      {"Total subresrouce count", "Tracked Subresource ID Limit", "Tracked free Subresouce IDs", "Tracked used Subresouce IDs"});
    auto limit = getGlobalSubresourceIdRangeLimit();
    auto freeCount = getFreeGlobalSubresourceIdRangeValues();

    char strBuf[32];
    sprintf_s(strBuf, "%I64u", subresouceCount);
    begin_selectable_row(strBuf);

    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%u", limit);

    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%u", freeCount);

    ImGui::TableSetColumnIndex(3);
    ImGui::Text("%u (%.2f %%)", limit - freeCount, (limit - freeCount) * 100.f / subresouceCount);
    ImGui::EndTable();
  }
}

void DebugView::drawBuffersTable()
{
  if (ImGui::BeginTable("DX12-buffer-heap-table", 9,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
  {
    ImGui::TableSetupScrollFreeze(0, 1);
    make_table_header( //
      {"Id", "Object", "GPU Address From", "CPU Pointer", "Can Suballoc", "Free Size", "Ready Size", "Allocated Size", "Total Size"});
    {
      auto bufferHeapStateAccess = bufferHeapState.access();
      for (const auto &heap : bufferHeapStateAccess->bufferHeaps)
      {
        char strBuf[32];
        sprintf_s(strBuf, "%u", heap.resId.index());

        if (heap)
        {
          ByteUnits freeSize = eastl::accumulate(begin(heap.freeRanges), end(heap.freeRanges), 0ull,
            [](uint64_t size, ValueRange<uint64_t> range) { return size + range.size(); });
          ByteUnits readySize = eastl::accumulate(begin(bufferHeapStateAccess->bufferHeapDiscardStandbyList),
            end(bufferHeapStateAccess->bufferHeapDiscardStandbyList), 0ull,
            [id = heap.resId.index()](size_t size, auto &info) { return size + ((info.index == id) ? info.range.size() : 0); });
          ByteUnits totalSize = heap.bufferMemory.size();
          ByteUnits allocatedSize = totalSize - freeSize - readySize;

          begin_selectable_row(strBuf);

          ImGui::TableNextColumn();
          ImGui::Text("%p", heap.buffer.Get());

          ImGui::TableNextColumn();
          ImGui::Text("%016I64X", heap.getGPUPointer());

          if (heap.getCPUPointer())
          {
            ImGui::TableNextColumn();
            ImGui::Text("%p", heap.getCPUPointer());
          }
          else
          {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("-");
          }

          ImGui::TableNextColumn();
          ImGui::TextUnformatted(heap.canSubAllocateFrom() ? "Yes" : "No");

          ImGui::TableNextColumn();
          ImGui::Text("%.2f %s (%.2f%%)", freeSize.units(), freeSize.name(), freeSize.value() * 100.0 / totalSize.value());

          ImGui::TableNextColumn();
          ImGui::Text("%.2f %s (%.2f%%)", readySize.units(), readySize.name(), readySize.value() * 100.0 / totalSize.value());

          ImGui::TableNextColumn();
          ImGui::Text("%.2f %s (%.2f%%)", allocatedSize.units(), allocatedSize.name(),
            allocatedSize.value() * 100.0 / totalSize.value());

          ImGui::TableNextColumn();
          ImGui::Text("%.2f %s", totalSize.units(), totalSize.name());
        }
        else
        {
          begin_selectable_row(strBuf);
          for (int i = 1; i < ImGui::TableGetColumnCount(); ++i)
          {
            ImGui::TableNextColumn();
            ImGui::TextDisabled("-");
          }
        }
      }
    }
    ImGui::EndTable();
  }

  if (ImGui::BeginTable("DX12-Buffer-Heap-Summary-Table", 6,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    ByteUnits gpuSize;
    ByteUnits gpuFreeSize;
    ByteUnits gpuUnuseableSize;
    ByteUnits systemSize;
    ByteUnits systemFreeSize;
    ByteUnits systemUnuseableSize;
    ByteUnits gpuStandbyAllocateSize;
    ByteUnits systemStandbyAllocateSize;

    {
      auto bufferHeapStateAccess = bufferHeapState.access();
      gpuStandbyAllocateSize = eastl::accumulate(begin(bufferHeapStateAccess->bufferHeapDiscardStandbyList),
        end(bufferHeapStateAccess->bufferHeapDiscardStandbyList), 0ull,
        [&bufferHeapStateAccess](size_t size, auto &info) //
        { return size + ((nullptr == bufferHeapStateAccess->bufferHeaps[info.index].getCPUPointer()) ? info.range.size() : 0); });
      systemStandbyAllocateSize = eastl::accumulate(begin(bufferHeapStateAccess->bufferHeapDiscardStandbyList),
        end(bufferHeapStateAccess->bufferHeapDiscardStandbyList), 0ull,
        [&bufferHeapStateAccess](size_t size, auto &info) //
        { return size + ((nullptr != bufferHeapStateAccess->bufferHeaps[info.index].getCPUPointer()) ? info.range.size() : 0); });

      for (const auto &heap : bufferHeapStateAccess->bufferHeaps)
      {
        if (heap)
        {
          auto freeMem = eastl::accumulate(begin(heap.freeRanges), end(heap.freeRanges), 0ull,
            [](size_t size, auto range) { return size + range.size(); });
          if (heap.getCPUPointer())
          {
            systemSize += heap.bufferMemory.size();
            if (heap.canSubAllocateFrom())
            {
              systemFreeSize += freeMem;
            }
            else
            {
              systemUnuseableSize += freeMem;
            }
          }
          else
          {
            gpuSize += heap.bufferMemory.size();
            if (heap.canSubAllocateFrom())
            {
              gpuFreeSize += freeMem;
            }
            else
            {
              gpuUnuseableSize += freeMem;
            }
          }
        }
      }
    }
    ByteUnits gpuAllocatedSize = gpuSize - gpuFreeSize - gpuStandbyAllocateSize;
    ByteUnits systemAllocatedSize = systemSize - systemFreeSize - systemStandbyAllocateSize;
    ByteUnits totalSize = gpuSize + systemSize;
    ByteUnits totalFreeSize = gpuFreeSize + systemFreeSize;
    ByteUnits totalStandbyAllocateSize = gpuStandbyAllocateSize + systemStandbyAllocateSize;
    ByteUnits totalUnusableSize = gpuUnuseableSize + systemUnuseableSize;
    ByteUnits totalAllocatedSize = gpuAllocatedSize + systemAllocatedSize;

    make_table_header( //
      {"Type", "Free Size", "Standby Size", "Unusable Size", "Allocated Size", "Total Size"});

    begin_selectable_row("GPU");
    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s (%.2f%%)", gpuFreeSize.units(), gpuFreeSize.name(), gpuFreeSize.value() * 100.0 / gpuSize.value());

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s (%.2f%%)", gpuStandbyAllocateSize.units(), gpuStandbyAllocateSize.name(),
      gpuStandbyAllocateSize.value() * 100.0 / gpuSize.value());

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s (%.2f%%)", gpuUnuseableSize.units(), gpuUnuseableSize.name(),
      gpuUnuseableSize.value() * 100.0 / gpuSize.value());

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s (%.2f%%)", gpuAllocatedSize.units(), gpuAllocatedSize.name(),
      gpuAllocatedSize.value() * 100.0 / gpuSize.value());

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s", gpuSize.units(), gpuSize.name());

    begin_selectable_row("System");
    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s (%.2f%%)", systemFreeSize.units(), systemFreeSize.name(),
      systemFreeSize.value() * 100.0 / systemSize.value());

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s (%.2f%%)", systemStandbyAllocateSize.units(), systemStandbyAllocateSize.name(),
      systemStandbyAllocateSize.value() * 100.0 / systemSize.value());

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s (%.2f%%)", systemUnuseableSize.units(), systemUnuseableSize.name(),
      systemUnuseableSize.value() * 100.0 / systemSize.value());

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s (%.2f%%)", systemAllocatedSize.units(), systemAllocatedSize.name(),
      systemAllocatedSize.value() * 100.0 / systemSize.value());

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s", systemSize.units(), systemSize.name());

    begin_selectable_row("Total");
    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s (%.2f%%)", totalFreeSize.units(), totalFreeSize.name(), totalFreeSize.value() * 100.0 / totalSize.value());

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s (%.2f%%)", totalStandbyAllocateSize.units(), totalStandbyAllocateSize.name(),
      totalStandbyAllocateSize.value() * 100.0 / totalSize.value());

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s (%.2f%%)", totalUnusableSize.units(), totalUnusableSize.name(),
      totalUnusableSize.value() * 100.0 / totalSize.value());

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s (%.2f%%)", totalAllocatedSize.units(), totalAllocatedSize.name(),
      totalAllocatedSize.value() * 100.0 / totalSize.value());

    ImGui::TableNextColumn();
    ImGui::Text("%.2f %s", totalSize.units(), totalSize.name());
    ImGui::EndTable();
  }
}

void DebugView::drawRaytracingInfoView()
{
  drawRaytraceTopStructureTable();
  drawRaytraceBottomStructureTable();
  drawRaytraceSummaryTable();
  drawRaytracePlot();
}

#if DX12_USE_ESRAM
void DebugView::drawESRamInfoView()
{
  if (ImGui::BeginTable("DX12-esram-layouts", 4,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
  {

    make_table_header({"Name", "Type", "Offset", "Pages"});

    WinAutoLock lock{esramMutex};
    for (size_t i = 0; i < layouts.size(); ++i)
    {
      auto &layout = layouts[i];
      const auto &name = layout.getMBString();
      begin_selectable_row(name.c_str());

      ImGui::TableSetColumnIndex(3);
      uint64_t usedPageCount = layout.layout->GetCurrentPageOffset();
      if (usedPageCount > esram_page_count)
      {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.75f, .75f, 0.f, 1.f));
      }
      ImGui::Text("%I64u (%.2f%%)", usedPageCount, usedPageCount * 100.0 / esram_page_count);
      if (usedPageCount > esram_page_count)
      {
        ImGui::PopStyleColor();
      }
      visitImageObjects([i](auto image) //
        {
          const auto &esramInfo = image->getEsramResource();
          if (esramInfo.mappingIndex == i)
          {
            image->getDebugName([](auto &name) { begin_selectable_row(name); });

            ImGui::TableSetColumnIndex(1);
            if (esramInfo.dramStorage)
            {
              ImGui::Text("<Mirrored> %p", esramInfo.dramStorage->getMemory().asPointer());
            }
            else
            {
              ImGui::TextUnformatted("<ESRam only>");
            }
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%u", esramInfo.mapping.StartPageIndex);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%u", esramInfo.mapping.PageCount);
          }
        });
    }
    ImGui::EndTable();
  }
}
#endif

void DebugView::drawTemporaryUploadMemoryInfoView()
{
  drawTemporaryUploadMemoryBasicMetrics();
  drawTempuraryUploadMemoryPlot();
  drawTempuraryUploadMemorySegmentsTable();
}

void DebugView::drawUploadRingBufferInfoView()
{
  drawUploadRingBasicMetricsTable();
  drawUploadRingMemoryPlot();
  drawUploadRingSegmentsTable();

  bool v = shouldTrimFramePushRingBuffer();
  ImGui::Checkbox("Trimming", &v);
}

void DebugView::drawConstRingBufferInfoView()
{
  drawConstRingBasicMetricsTable();
  drawConstRingMemoryPlot();
  drawConstRingSegmentsTable();

  bool v = shouldTrimUploadRingBuffer();
  ImGui::Checkbox("Trimming", &v);
}

void DebugView::drawBuffersInfoView()
{
  drawBuffersTable();
  drawBuffersPlot();
}

void DebugView::drawTexturesInfoView()
{
  drawTextureTable();
  drawTexturePlot();
}

void DebugView::drawScratchBufferInfoView()
{
  drawScratchBufferTable();
  drawScratchBufferPlot();
}

void DebugView::drawUserHeapInfoView()
{
  drawUserHeapsTable();
  drawUserHeapsPlot();
  drawUserHeapsSummaryTable();
}

void DebugView::drawHeapsTable()
{
  constexpr int table_height = 15;
  if (ImGui::BeginTable("DX12-Heap-Table", 7,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
        ImVec2(0, table_height * ImGui::GetFrameHeightWithSpacing())))
  {
    make_table_header( //
      {"Identifier", "Children", "Placement Start", "Placement End", "Allocated Size", "Free Size", "Total Size"});
    ResourceHeapProperties properties;
    {
      // have to get access to buffer heaps before to heaps as otherwise we have a ordering issue and deadlock.
      auto bufferHeapStateAccess = bufferHeapState.access();
      OSSpinlockScopedLock lock{heapGroupMutex};
      for (properties.raw = 0; properties.raw < groups.size(); ++properties.raw)
      {
        auto &group = groups[properties.raw];
        FragmentationCalculatorContext overalFragmentation;
        ByteUnits totalSize;
        ByteUnits totalFree;
        size_t children = 0;
        size_t activHeapCount = 0;
        for (auto &heap : group)
        {
          if (!heap)
          {
            continue;
          }
          totalSize += heap.totalSize;
          totalFree +=
            eastl::accumulate(begin(heap.freeRanges), end(heap.freeRanges), size_t(0), [](auto a, auto b) { return a + b.size(); });
          children += heap.usedRanges.size();
          overalFragmentation.fragments(heap.freeRanges);
          ++activHeapCount;
        }

        ByteUnits totalAllocated = totalSize - totalFree;

#if _TARGET_XBOX
#if _TARGET_SCARLETT
        const char *heapTypePropertyName = properties.isGPUOptimal ? "GPU Optimal Bandwidth" : "Standard Bandwidth";
#else
        const char *heapTypePropertyName = "Standard Bandwidth";
#endif
        const char *cpuCachePropertyName = properties.isCPUCoherent ? "CPU Coherent" : "CPU Write Combine";
        const char *miscHeapPropertyName = properties.isGPUExecutable ? "GPU Executable Read Write" : "GPU Read Write";
        uint32_t heapFlags = properties.raw;
#else
        const char *heapTypePropertyName = to_string(properties.getMemoryPool(isUMASystem()));
        const char *cpuCachePropertyName = to_string(properties.getCpuPageProperty(isUMASystem()));
        const char *miscHeapPropertyName = to_string(properties.getHeapType());
        uint32_t heapFlags = static_cast<uint32_t>(properties.getFlags(canMixResources()));
#endif

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        char strBuf[256];
        sprintf_s(strBuf, "%1u - 0x%08X <%s> <%s> <%s>", properties.raw, heapFlags, heapTypePropertyName, cpuCachePropertyName,
          miscHeapPropertyName);
        bool open = ImGui::TreeNodeEx(strBuf, ImGuiTreeNodeFlags_SpanFullWidth);

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%zu | %zu", activHeapCount, children);

        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%.2f %s", totalAllocated.units(), totalAllocated.name());

        ImGui::TableSetColumnIndex(5);
        ImGui::Text("%.2f %s (%.2f %% fragmentation)", totalFree.units(), totalFree.name(), overalFragmentation.valuePrecise());

        ImGui::TableSetColumnIndex(6);
        ImGui::Text("%.2f %s", totalSize.units(), totalSize.name());
        if (open)
        {
          for (auto heap = begin(group), hend = end(group); heap != hend; ++heap)
          {
            if (!*heap)
            {
              continue;
            }
            ByteUnits sizeUnit = heap->totalSize;
            ByteUnits totalFree = eastl::accumulate(begin(heap->freeRanges), end(heap->freeRanges), size_t(0),
              [](auto a, auto b) { return a + b.size(); });
            ByteUnits totalAllocated = heap->totalSize - totalFree;
            auto fragmentation = free_list_calculate_fragmentation_precise(heap->freeRanges);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            sprintf_s(strBuf, "%p", heap->heapPointer());
            open = ImGui::TreeNodeEx(strBuf, ImGuiTreeNodeFlags_SpanFullWidth);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", heap->usedRanges.size());

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%.2f %s", totalAllocated.units(), totalAllocated.name());

            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%.2f %s (%.2f %% fragmentation)", totalFree.units(), totalFree.name(), fragmentation);

            ImGui::TableSetColumnIndex(6);
            ImGui::Text("%.2f %s", sizeUnit.units(), sizeUnit.name());

            if (open)
            {
              for (BasicResourceHeapRangesIterator segment{*heap}; segment; ++segment)
              {
                if (segment.isFreeRange())
                {
                  draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Free");
                }
                else
                {
                  auto &res = segment.getUsedResource();
                  if (eastl::holds_alternative<Image *>(res))
                  {
                    eastl::get<Image *>(res)->getDebugName(
                      [heap, &segment](auto &res_name) { draw_segment(heap->heapPointer(), *segment, heap->totalSize, res_name); });
                  }
                  else if (eastl::holds_alternative<BufferGlobalId>(res))
                  {
                    auto id = eastl::get<BufferGlobalId>(res);
                    auto &buffer = bufferHeapStateAccess->bufferHeaps[id.index()];
                    auto standbyRef = eastl::find_if(begin(bufferHeapStateAccess->bufferHeapDiscardStandbyList),
                      end(bufferHeapStateAccess->bufferHeapDiscardStandbyList),
                      [idx = id.index()](auto info) { return idx == info.index; });
                    if (standbyRef == end(bufferHeapStateAccess->bufferHeapDiscardStandbyList))
                    {
                      char resnameBuffer[MAX_OBJECT_NAME_LENGTH];
                      draw_segment(heap->heapPointer(), *segment, heap->totalSize,
                        get_resource_name(buffer.buffer.Get(), resnameBuffer));
                    }
                    else
                    {
                      draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Standby for discard");
                    }
                  }
                  else if (eastl::holds_alternative<AliasHeapReference>(res))
                  {
                    // auto aliasHeapRef = eastl::get<AliasHeapReference>(res);
                    // auto &heap = aliasHeaps[aliasHeapRef.index];
                    draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Aliasing Heap");
                    // TODO list current resources
                  }
                  else if (eastl::holds_alternative<ScratchBufferReference>(res))
                  {
                    draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Scratch buffer");
                  }
                  else if (eastl::holds_alternative<PushRingBufferReference>(res))
                  {
                    draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Push ring buffer");
                  }
                  else if (eastl::holds_alternative<UploadRingBufferReference>(res))
                  {
                    draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Upload ring buffer");
                  }
                  else if (eastl::holds_alternative<TempUploadBufferReference>(res))
                  {
                    draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Temp upload buffer");
                  }
                  else if (eastl::holds_alternative<PersistentUploadBufferReference>(res))
                  {
                    draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Persistent upload buffer");
                  }
                  else if (eastl::holds_alternative<PersistentReadBackBufferReference>(res))
                  {
                    draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Persistent read back buffer");
                  }
                  else if (eastl::holds_alternative<PersistentBidirectionalBufferReference>(res))
                  {
                    draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Persistent bidirectional buffer");
                  }
                  else if (eastl::holds_alternative<RaytraceAccelerationStructureHeapReference>(res))
                  {
                    draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Raytracing acceleration structure heap");
                  }
                  else
                  {
                    if (eastl::holds_alternative<eastl::monostate>(res))
                    {
                      draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Allocated (missing resource info)");
                    }
                    else
                    {
                      draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Allocated (missing display handling!)");
                    }
                  }
                }
              }
              ImGui::TreePop();
            }
          }
          ImGui::TreePop();
        }
      }
    }
    ImGui::EndTable();
  }
}

void DebugView::drawHeapInfoView()
{
  drawHeapsTable();
  drawHeapsPlot();
  drawHeapsSummaryTable();
}

void DebugView::drawSystemInfoView()
{
  if (ImGui::BeginTable("DX12-Device-Caps-Table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    make_table_header({"Capability", "Support"});

    begin_selectable_row("UMA System");
    if (ImGui::IsItemHovered())
    {
      ScopedTooltip tooltip;
      ImGui::TextUnformatted("Specifies whether the hardware and driver support UMA. The runtime "
                             "sets this member to TRUE if the hardware and driver support UMA.");
    }
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(isUMASystem() ? "Yes" : "No");

    begin_selectable_row("Can mix resource types in memory heaps");
    if (ImGui::IsItemHovered())
    {
      ScopedTooltip tooltip;
      ImGui::TextUnformatted("Devices that can not mix resource types in memory heaps, have to use "
                             "different memory heaps for different resource types. There are 3 "
                             "resource types, buffers, render / depth-stencil targets and "
                             "textures.");
    }
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(canMixResources() ? "Yes" : "No");

    ImGui::EndTable();
  }

  drawSystemCurrentMemoryUseTable();
  drawSystemMemoryUsePlot();
}

void DebugView::drawManagerBehaviorInfoView()
{
  uint64_t minOffsetSize = 0;
  uint64_t maxOffsetSize = 0;
  if (!isUMASystem())
  {
    maxOffsetSize = getDeviceLocalRawBudget() - 1;
    uint64_t offset = getDeviceLocalHeapBudgetOffset();
    ImGui::SliderScalar("Device Memory Budged Offset", ImGuiDataType_U64, &offset, &minOffsetSize, &maxOffsetSize, nullptr,
      ImGuiSliderFlags_AlwaysClamp);
    setDeviceLocalHeapBudgetOffset(offset);
  }

  {
    maxOffsetSize = getHostLocalRawBudget() - 1;
    uint64_t offset = getHostLocalHeapBudgetOffset();
    ImGui::SliderScalar("Host Memory Budged Offset", ImGuiDataType_U64, &offset, &minOffsetSize, &maxOffsetSize, nullptr,
      ImGuiSliderFlags_AlwaysClamp);
    setHostLocalHeapBudgetOffset(offset);
  }

  auto as_color = [](BudgetPressureLevels l) {
    switch (l)
    {
      case BudgetPressureLevels::PANIC: return ImVec4{1.f, 0.f, 0.f, 1.f};
      case BudgetPressureLevels::HIGH: return ImVec4{.75f, 0.f, 0.f, 1.f};
      case BudgetPressureLevels::MEDIUM: return ImVec4{.75f, .75f, 0.f, 1.f};
      default:
      case BudgetPressureLevels::LOW: return ImVec4{.0f, .75f, 0.f, 1.f};
    }
  };

  ImGui::BeginTable("DX12-Memory-Manager-Info-Metric-Table", 2,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable/* |
                      ImGuiTableFlags_ScrollY*/);

  make_table_header({"Metric", "Value"});
  if (!isUMASystem())
  {
    begin_selectable_row("Adjusted Device Memory Budged");
    ImGui::TableNextColumn();
    ByteUnits budget = getDeviceLocalBudget();
    ImGui::Text("%.2f %s", budget.units(), budget.name());

    begin_selectable_row("Adjusted Available Device Memory Budged");
    ImGui::TableNextColumn();
    budget = getDeviceLocalAvailablePoolBudget();
    ImGui::Text("%.2f %s", budget.units(), budget.name());
  }

  {
    begin_selectable_row("Adjusted Host Memory Budged");
    ImGui::TableNextColumn();
    ByteUnits budget = getHostLocalBudget();
    ImGui::Text("%.2f %s", budget.units(), budget.name());

    begin_selectable_row("Adjusted Available Host Memory Budged");
    ImGui::TableNextColumn();
    budget = getHostLocalAvailablePoolBudget();
    ImGui::Text("%.2f %s", budget.units(), budget.name());
  }

  if (!isUMASystem())
  {
    begin_selectable_row("Device Memory Budged Level");
    ImGui::TableNextColumn();
    {
      ScopedStyleColor style{ImGuiCol_Text, as_color(getDeviceLocalBudgetLevel())};
      ImGui::TextUnformatted(as_string(getDeviceLocalBudgetLevel()));
    }
  }

  {
    begin_selectable_row("Host Memory Budged Level");
    ImGui::TableNextColumn();
    {
      ScopedStyleColor style{ImGuiCol_Text, as_color(getHostLocalBudgetLevel())};
      ImGui::TextUnformatted(as_string(getHostLocalBudgetLevel()));
    }
  }

  if (!isUMASystem())
  {
    begin_selectable_row("Device Memory Panic Budget Limit");
    ImGui::TableNextColumn();
    ByteUnits limit = getDeviceLocalBudgetLimit(BudgetPressureLevels::PANIC);
    ImGui::Text("%.2f %s", limit.units(), limit.name());

    begin_selectable_row("Device Memory High Budget Limit");
    ImGui::TableNextColumn();
    limit = getDeviceLocalBudgetLimit(BudgetPressureLevels::HIGH);
    ImGui::Text("%.2f %s", limit.units(), limit.name());

    begin_selectable_row("Device Memory Medium Budget Limit");
    ImGui::TableNextColumn();
    limit = getDeviceLocalBudgetLimit(BudgetPressureLevels::MEDIUM);
    ImGui::Text("%.2f %s", limit.units(), limit.name());
  }

  {
    begin_selectable_row("Host Memory Panic Budget Limit");
    ImGui::TableNextColumn();
    ByteUnits limit = getHostLocalBudgetLimit(BudgetPressureLevels::PANIC);
    ImGui::Text("%.2f %s", limit.units(), limit.name());

    begin_selectable_row("Host Memory High Budget Limit");
    ImGui::TableNextColumn();
    limit = getHostLocalBudgetLimit(BudgetPressureLevels::HIGH);
    ImGui::Text("%.2f %s", limit.units(), limit.name());

    begin_selectable_row("Host Memory Medium Budget Limit");
    ImGui::TableNextColumn();
    limit = getHostLocalBudgetLimit(BudgetPressureLevels::MEDIUM);
    ImGui::Text("%.2f %s", limit.units(), limit.name());
  }

  ImGui::EndTable();
}

void DebugView::drawEventsInfoView()
{
  drawMetricsEventsView();
  drawMetricsEvnetsViewFilterControls();
}

void DebugView::drawMetricsInfoView() { drawMetricsCaptureControls(); }

void DebugView::drawSamplersInfoView() { drawSamplerTable(); }

namespace
{
const char *to_string(D3D12_FILTER_TYPE filter)
{
  switch (filter)
  {
    case D3D12_FILTER_TYPE_POINT: return "point";
    case D3D12_FILTER_TYPE_LINEAR: return "linear";
    default: return "<unknown>";
  }
}
const char *to_string(D3D12_TEXTURE_ADDRESS_MODE am)
{
  switch (am)
  {
    case D3D12_TEXTURE_ADDRESS_MODE_WRAP: return "wrap";
    case D3D12_TEXTURE_ADDRESS_MODE_MIRROR: return "mirror";
    case D3D12_TEXTURE_ADDRESS_MODE_CLAMP: return "clamp";
    case D3D12_TEXTURE_ADDRESS_MODE_BORDER: return "border";
    case D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE: return "mirror once";
    default: return "<unknown>";
  }
}
} // namespace

void DebugView::drawSamplerTable()
{
  uint32_t samplerCount = 0;
  if (ImGui::BeginTable("DX12-Sampler-Table", 10,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
  {
    make_table_header({"Is Compare", "Filter Mode", "MipMap Mode", "U Address Mode", "V Address Mode", "W Address Mode", "Load Bias",
      "Anisotropic Max", "Border Color", "Compressed State Value"});

    visitSamplerObjects([&samplerCount](auto, const auto &info) {
      ++samplerCount;

      char strBuf[64];
      sprintf_s(strBuf, "%u", static_cast<uint32_t>(info.state.isCompare));
      begin_selectable_row(strBuf);

      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%s", to_string(info.state.getFilter()));

      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%s", to_string(info.state.getMip()));

      ImGui::TableSetColumnIndex(3);
      ImGui::Text("%s", to_string(info.state.getU()));

      ImGui::TableSetColumnIndex(4);
      ImGui::Text("%s", to_string(info.state.getV()));

      ImGui::TableSetColumnIndex(5);
      ImGui::Text("%s", to_string(info.state.getW()));

      ImGui::TableSetColumnIndex(6);
      ImGui::Text("%f", info.state.getBias());

      ImGui::TableSetColumnIndex(7);
      ImGui::Text("%f", info.state.getAniso());

      ImGui::TableSetColumnIndex(8);
      ImGui::Text("%X", static_cast<unsigned int>(info.state.getBorder()));

      ImGui::TableSetColumnIndex(9);
      ImGui::Text("%08X", info.state.wrapper.value);
    });
    ImGui::EndTable();
  }

  if (ImGui::BeginTable("DX12-Sampler-Summary-Table", 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
  {
    make_table_header({"Sampler Counter"});
    char strBuf[32];
    sprintf_s(strBuf, "%u", samplerCount);
    begin_selectable_row(strBuf);
    ImGui::EndTable();
  }
}

void DebugView::debugOverlay()
{
  using eastl::begin;
  using eastl::end;

#if _TARGET_PC_WIN
  if (!get_device().isInitialized())
  {
    show_wrong_driver_info();
    return;
  }
#endif
  // ImPlot is a mess, division by 0, multiply by inf and other exceptions left and right...
  FloatingPointExceptionsKeeper disableFE;

  if (checkStatusFlag(StatusFlag::LOAD_VIEWS_FROM_CONFIG))
  {
    restoreViewSettings();
    setStatusFlag(StatusFlag::LOAD_VIEWS_FROM_CONFIG, false);
  }

  if (ImGui::TreeNodeEx("Live Metrics##DX12-Live-Metrics", ImGuiTreeNodeFlags_Framed))
  {
    drawMetricsInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Events##DX12-Events", ImGuiTreeNodeFlags_Framed))
  {
    drawEventsInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Manager Behavior##DX12-Resource-Memory-Manager-Behavior", ImGuiTreeNodeFlags_Framed))
  {
    drawManagerBehaviorInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("System Overview##DX12-System-Overview", ImGuiTreeNodeFlags_Framed))
  {
    drawSystemInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Resource Memory Heaps##DX12-Resource-Memory-Heaps", ImGuiTreeNodeFlags_Framed))
  {
    drawHeapInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("User Resource Heaps##DX12-User-Resource-Heaps", ImGuiTreeNodeFlags_Framed))
  {
    drawUserHeapInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Scratch Buffer##DX12-Scratch-Buffer", ImGuiTreeNodeFlags_Framed))
  {
    drawScratchBufferInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Textures##DX12-Textures", ImGuiTreeNodeFlags_Framed))
  {
    drawTexturesInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Samplers##DX12-Samplers", ImGuiTreeNodeFlags_Framed))
  {
    drawSamplersInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Buffers##DX12-Buffers", ImGuiTreeNodeFlags_Framed))
  {
    drawBuffersInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Const push ring buffer##DX12-const-push-ring-buffer", ImGuiTreeNodeFlags_Framed))
  {
    drawConstRingBufferInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Upload ring buffer##DX12-upload-ring-buffer", ImGuiTreeNodeFlags_Framed))
  {
    drawUploadRingBufferInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Temporary upload memory##DX12-temporary-upload-memory", ImGuiTreeNodeFlags_Framed))
  {
    drawTemporaryUploadMemoryInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Persistent upload memory##DX12-persistent-upload-memory", ImGuiTreeNodeFlags_Framed))
  {
    drawPersistenUploadMemoryInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Persistent read back memory##DX12-persistent-read-back-memory", ImGuiTreeNodeFlags_Framed))
  {
    drawPersistenReadBackMemoryInfoView();
    ImGui::TreePop();
  }

  if (ImGui::TreeNodeEx("Persistent bidirectional memory##DX12-persistent-bidirectional-memory", ImGuiTreeNodeFlags_Framed))
  {
    drawPersistenBidirectioanlMemoryInfoView();
    ImGui::TreePop();
  }

#if DX12_USE_ESRAM
  if (ImGui::TreeNodeEx("ESRam Usage##DX12-ESRam-Usage", ImGuiTreeNodeFlags_Framed))
  {
    drawESRamInfoView();
    ImGui::TreePop();
  }
#endif

  if (ImGui::TreeNodeEx("Raytracing##DX12-raytracing", ImGuiTreeNodeFlags_Framed))
  {
    drawRaytracingInfoView();
    ImGui::TreePop();
  }

  if (checkStatusFlag(StatusFlag::STORE_VIEWS_TO_CONFIG))
  {
    storeViewSettings();
    setStatusFlag(StatusFlag::STORE_VIEWS_TO_CONFIG, false);
  }
}
#endif

#if DX12_SUPPORT_RESOURCE_MEMORY_METRICS
#include <perfMon/dag_memoryReport.h>

void MetricsProvider::setup(const SetupInfo &info)
{
  metricsCollectedBits = info.collectedMetrics;
  BaseType::setup(info);

#if _TARGET_PC_WIN
  memoryreport::register_vram_state_callback([this](memoryreport::VRamState &vram_state) {
    auto metricsAccess = accessMetrics();

    const auto &frameCounters = metricsAccess->getLastFrameMetrics();

    vram_state.used_device_local = frameCounters.deviceMemoryInUse.CurrentUsage;
    vram_state.used_shared = frameCounters.systemMemoryInUse.CurrentUsage;
  });
#endif
}

void MetricsProvider::shutdown()
{
  {
    *metrics.access() = {};
  }
  memoryreport::register_vram_state_callback({});

  BaseType::shutdown();
}
#endif

#if DX12_USE_ESRAM
void resource_manager::ESRamPageMappingProvider::fetchMovableTextures(DeviceContext &context)
{
  WinAutoLock lock{esramMutex};
  for (Image *tex : movableTextures)
  {
    context.copyImage(tex->getEsramResource().dramStorage, tex, make_whole_resource_copy_info());
  }
}

void resource_manager::ESRamPageMappingProvider::writeBackMovableTextures(DeviceContext &context)
{
  WinAutoLock lock{esramMutex};
  for (Image *tex : movableTextures)
  {
    context.copyImage(tex, tex->getEsramResource().dramStorage, make_whole_resource_copy_info());
  }
}
#endif

namespace
{
const char *filter_type_to_string(D3D12_FILTER_TYPE type)
{
  switch (type)
  {
    default: return "<error>";
    case D3D12_FILTER_TYPE_POINT: return "D3D12_FILTER_TYPE_POINT";
    case D3D12_FILTER_TYPE_LINEAR: return "D3D12_FILTER_TYPE_LINEAR";
  }
}

template <typename T>
struct ResourceHeapReportVisitor
{
  T &target;
  size_t totalHeapCount = 0;
  ByteUnits totalResourceMemorySize;
  ByteUnits freeResourceMemorySize;

  void beginVisit() { target("~ Resource Heaps ~~~~~~~~~~~~~~~~~~~~"); }

  void endVisit()
  {
    target("%u resource heaps, with %6.2f %7s in total and %6.2f %7s free", totalHeapCount, totalResourceMemorySize.units(),
      totalResourceMemorySize.name(), freeResourceMemorySize.units(), freeResourceMemorySize.name());
  }

  void visitHeapGroup(uint32_t ident, size_t count, bool is_cpu_visible, bool is_cpu_cached, bool is_gpu_executable)
  {
    target("Heap Group %08X (%s, %s%s) with %d heaps", ident, is_cpu_visible ? "CPU visible" : "dedicated GPU",
      is_cpu_visible ? is_cpu_cached ? "CPU cached, " : "CPU write combine, " : "",
      is_gpu_executable ? "GPU executable" : "Not GPU executable", count);
    totalHeapCount += count;
  }

  void visitHeap(ByteUnits total_size, ByteUnits free_size, uint32_t fragmentation_percent, uintptr_t)
  {
    totalResourceMemorySize += total_size;
    freeResourceMemorySize += free_size;
    target("Heap with %6.2f %7s, %6.2f %7s free, %3u%% fragmentation", total_size.units(), total_size.name(), free_size.units(),
      free_size.name(), fragmentation_percent);
  }

  template <typename T>
  void visitResourceInHeap(ValueRange<uint64_t> range, const T &)
  {
    ByteUnits size = range.size();
    target("%016llX with %6.2f %7s Generic resource (missing type handler)", range.front(), size.units(), size.name());
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, eastl::monostate)
  {
    // rare case when the memory got allocated but the resource that uses it is currently created and we don't know
    // anything about it yet.
    ByteUnits size = range.size();
    target("%016llX with %6.2f %7s in flight resource creation, no resource info yet", range.front(), size.units(), size.name());
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, Image *tex)
  {
    tex->getDebugName([this, range, tex](const auto &name) {
      ByteUnits size = range.size();
      target("%016llX with %6.2f %7s Texture %016p <%s>", range.front(), size.units(), size.name(), tex, name);
    });
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, BufferGlobalId buffer)
  {
    ByteUnits size = range.size();
    target("%016llX with %6.2f %7s Buffer #%u", range.front(), size.units(), size.name(), buffer.index());
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, const ResourceMemoryHeap::RaytraceAccelerationStructureHeapReference &)
  {
    ByteUnits size = range.size();
    target("%016llX with %6.2f %7s Accel Struct Heap", range.front(), size.units(), size.name());
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, const ResourceMemoryHeap::AliasHeapReference &a_heap)
  {
    ByteUnits size = range.size();
    target("%016llX with %6.2f %7s Aliasing Heap #%u", range.front(), size.units(), size.name(), a_heap.index);
    // TODO drill down user heap?
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, const ResourceMemoryHeap::ScratchBufferReference &)
  {
    ByteUnits size = range.size();
    target("%016llX with %6.2f %7s Scratch buffer", range.front(), size.units(), size.name());
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, const ResourceMemoryHeap::PushRingBufferReference &)
  {
    ByteUnits size = range.size();
    target("%016llX with %6.2f %7s Push ring", range.front(), size.units(), size.name());
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, const ResourceMemoryHeap::UploadRingBufferReference &)
  {
    ByteUnits size = range.size();
    target("%016llX with %6.2f %7s Upload ring", range.front(), size.units(), size.name());
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, const ResourceMemoryHeap::TempUploadBufferReference &)
  {
    ByteUnits size = range.size();
    target("%016llX with %6.2f %7s Temp upload ring", range.front(), size.units(), size.name());
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, const ResourceMemoryHeap::PersistentUploadBufferReference &)
  {
    ByteUnits size = range.size();
    target("%016llX with %6.2f %7s Persistent upload buffer", range.front(), size.units(), size.name());
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, const ResourceMemoryHeap::PersistentReadBackBufferReference &)
  {
    ByteUnits size = range.size();
    target("%016llX with %6.2f %7s Persistent read back buffer", range.front(), size.units(), size.name());
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, const ResourceMemoryHeap::PersistentBidirectionalBufferReference &)
  {
    ByteUnits size = range.size();
    target("%016llX with %6.2f %7s Persistent bidirectional buffer", range.front(), size.units(), size.name());
  }

  void visitHeapUsedRange(ValueRange<uint64_t> range, const ResourceMemoryHeap::AnyResourceReference &res)
  {
    eastl::visit([this, range](const auto &e) { visitResourceInHeap(range, e); }, res);
  }

  void visitHeapFreeRange(ValueRange<uint64_t> range)
  {
    ByteUnits sizeBytes{range.size()};
    target("%016llX with %6.2f %7s Free", range.front(), sizeBytes.units(), sizeBytes.name());
  }
};

struct HeapDescPair
{
  uint64_t heapOffset;
  uintptr_t heapID;
};

struct ResourceHeapDumpVisitor
{
  Tab<ResourceDumpInfo> &dumpInfo;
  eastl::vector_map<Image *, HeapDescPair> &imgAdressMap;
  eastl::vector_map<uint32_t, HeapDescPair> &buffersPresent;
  uintptr_t resourceHeapId = 0;

  template <typename T>
  void visitResourceInHeap(ValueRange<uint64_t>, const T &)
  {}

  void visitResourceInHeap(ValueRange<uint64_t> range, Image *img)
  {
    imgAdressMap.insert(eastl::make_pair(img, HeapDescPair({range.front(), resourceHeapId})));
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, BufferGlobalId b)
  {
    buffersPresent.insert(eastl::make_pair(b.index(), HeapDescPair({range.front(), resourceHeapId})));
  }

  void visitResourceInHeap(ValueRange<uint64_t> range, const ResourceMemoryHeap::RaytraceAccelerationStructureHeapReference &as)
  {
    ByteUnits size = range.size();

    dumpInfo.emplace_back(
      ResourceDumpRayTrace({as.as->GetGPUVirtualAddress(), (uint64_t)resourceHeapId, range.front(), (int)size.value(), true, false}));
  }

  void visitHeapUsedRange(ValueRange<uint64_t> range, const ResourceMemoryHeap::AnyResourceReference &res)
  {
    eastl::visit([this, range](const auto &e) { visitResourceInHeap(range, e); }, res);
  }

  void visitHeapFreeRange(ValueRange<uint64_t>) {}
  void beginVisit() {}
  void endVisit() {}
  void visitHeapGroup(uint32_t, size_t, bool, bool, bool) {}
  void visitHeap(ByteUnits, ByteUnits, uint32_t, uintptr_t heapId) { resourceHeapId = heapId; }
};

template <typename T>
struct BufferHeapReportVisitor
{
  T &target;
  size_t bufferCount = 0;
  size_t standbyCount = 0;
  ByteUnits internalBufferTotalSize;
  ByteUnits internalBufferFreeSize;
  ByteUnits standbyTotalSize;

  void beginVisit() {}
  void endVisit() {}

  void beginVisitBuffers()
  {
    target("~ All driver level buffers ~ (ID Order) ~~~~~~~~~");
    target(" ID,     Total size,      Free size, Has standby regions, Flags");
  }

  void endVisitBuffers()
  {
    target("%u buffers, with %6.2f %7s in total and %6.2f %7s free", bufferCount, internalBufferTotalSize.units(),
      internalBufferTotalSize.name(), internalBufferFreeSize.units(), internalBufferFreeSize.name());
  }

  void beginVisitStandbyRange()
  {
    target("~ Standby regions of buffers (Random order) ~~~~~~~~~");
    target(" ID,           Size,           Offset, Progress Counter");
  }

  void endVisitStandbyRange()
  {
    target("%u standby regions, with %6.2f %7s in total", standbyCount, standbyTotalSize.units(), standbyTotalSize.name());
  }

  void visitBuffer(uint32_t id, ByteUnits size, ByteUnits free_size, D3D12_RESOURCE_FLAGS flags, bool is_on_standby_list)
  {
    ++bufferCount;
    internalBufferTotalSize += size;
    internalBufferFreeSize += free_size;
    target("%3u, %6.2f %7s, %6.2f %7s,           %-9s, %08X", id, size.units(), size.name(), free_size.units(), free_size.name(),
      is_on_standby_list ? "X" : "0", flags);
  }
  void visitBufferFreeRange(ValueRange<uint64_t> range) { G_UNUSED(range); }
  void visitBufferStandbyRange(uint64_t progress, uint32_t id, ValueRange<uint64_t> range)
  {
    ++standbyCount;
    ByteUnits size = range.size();
    standbyTotalSize += range.size();
    target("%3u, %6.2f %7s, %016llX, %llu", id, size.units(), size.name(), range.front(), progress);
  }
};

template <typename T>
struct SbufferReportVisitor
{
  T &target;

  dag::Vector<const GenericBufferInterface *> buffers;

  void beginVisit(size_t max_size) { buffers.reserve(max_size); }

  void endVisit()
  {
    eastl::sort(begin(buffers), end(buffers), [](auto l, auto r) {
      if (l->ressize() > r->ressize())
      {
        return true;
      }
      if (l->ressize() < r->ressize())
      {
        return false;
      }
      return l < r;
    });

    target("~ Sbuffer resources ~ (Ressize Order) ~~~~~~~~~~~~~~~~");
    target("     Ressize, Allocated Size, Sys Copy Size, Dynamic, Internal ID,           Offset,  Discards,   Mapped Address, Name");
    ByteUnits publicSize;
    ByteUnits bufferTotalSize;
    ByteUnits bufferSysCopySize;

    for (auto buffer : buffers)
    {
      const BufferState &deviceBuffer = buffer->getDeviceBuffer();
      ByteUnits size = deviceBuffer.totalSize();
      publicSize += buffer->ressize();
      bufferTotalSize += size;
      ByteUnits copySize;
      if (buffer->hasSystemCopy())
      {
        copySize = buffer->ressize();
        bufferSysCopySize += copySize.value();
      }
      target("%12llu, %6.2f %7s, %6.2f %7s,   %s   , #%-10u, %016llX, %4u/%-4u, %016p, %s", buffer->ressize(), size.units(),
        size.name(), copySize.units(), copySize.name(), buffer->getFlags() & SBCF_DYNAMIC ? "X" : "0", deviceBuffer.resourceId.index(),
        deviceBuffer.offset, deviceBuffer.currentDiscardIndex, deviceBuffer.discardCount, deviceBuffer.currentCPUPointer(),
        buffer->getBufName());
    }

    target("%u buffers, with %6.2f %7s external size, with %6.2f %7s allocated size and %6.2f %7s system memory", buffers.size(),
      publicSize.units(), publicSize.name(), bufferTotalSize.units(), bufferTotalSize.name(), bufferSysCopySize.units(),
      bufferSysCopySize.name());
  }

  void visitBuffer(const GenericBufferInterface *buffer) { buffers.push_back(buffer); }
};

struct SbufferDumpVisitor
{
  Tab<ResourceDumpInfo> &dumpInfo;
  eastl::vector_map<uint32_t, HeapDescPair> &buffersPresent;
  void beginVisit(size_t) {}
  void endVisit() {}
  void visitBuffer(const GenericBufferInterface *buffer)
  {
    const BufferState &deviceBuffer = buffer->getDeviceBuffer();

    uint64_t heapId = (uint64_t)-1;
    uint64_t heapAddress = (uint64_t)-1;

    if (auto it = buffersPresent.find(deviceBuffer.resourceId.index()); it != buffersPresent.end())
    {
      heapId = (uint64_t)it->second.heapID;
      heapAddress = (uint64_t)it->second.heapOffset;
    }

    dumpInfo.emplace_back(
      ResourceDumpBuffer({(uint64_t)deviceBuffer.currentGPUPointer() == 0 ? (uint64_t)-1 : (uint64_t)deviceBuffer.currentGPUPointer(),
        heapId, heapAddress, deviceBuffer.offset,
        (uint64_t)deviceBuffer.currentCPUPointer() == 0 ? (uint64_t)-1 : (uint64_t)deviceBuffer.currentCPUPointer(), buffer->ressize(),
        buffer->hasSystemCopy() ? buffer->ressize() : -1, buffer->getFlags(), (int)deviceBuffer.currentDiscardIndex, -1,
        (int)deviceBuffer.discardCount, (int)deviceBuffer.resourceId.index(), buffer->getBufName(), ""}));
  }
};

template <typename T>
struct ImageReportVisitor
{
  T &target;
  size_t imageCount = 0;
  size_t imageAliases = 0;
  ByteUnits totalImageResourcesSize;

  void beginVisit()
  {
    target("~ All driver level textures ~ (Address Order) ~~~~~~~~");
    target("          Size,          Address, Name");
  }

  void endVisit()
  {

    target("%u textures, with %6.2f %7s plus %u aliased textures", imageCount, totalImageResourcesSize.units(),
      totalImageResourcesSize.name(), imageAliases);
  }

  void visitTexture(Image *image)
  {
    if (image->isAliased())
    {
      ++imageAliases;
      image->getDebugName([this, image](auto &name) { target("%14s, %016p, %s", "Aliased", image, name); });
    }
    else
    {
      ByteUnits size = image->getMemory().size();
      totalImageResourcesSize += size;
      image->getDebugName([this, image, size](auto &name) { target("%6.2f %7s, %016p, %s", size.units(), size.name(), image, name); });
    }
    ++imageCount;
  }
};

const char *skip_chars(const char *text, uint32_t count) { return text + count; }

template <typename T>
struct BaseTexReportVisitor
{
  T &target;
  uint32_t &tex2DCount;
  uint32_t &texCubeCount;
  uint32_t &texVolCount;
  uint32_t &texArrayCount;
  uint32_t &aliasedCount;
  ByteUnits &tex2DSize;
  ByteUnits &texCubeSize;
  ByteUnits &texVolSize;
  ByteUnits &texArraySize;
  String tmpStor;

  dag::Vector<const BaseTex *> textures;

  void beginVisit()
  {
    target("~ Texture resources ~ (Ressize Order) ~~~~~~~~~~~~~~~~");
    target("     Ressize, Allocated Size, QL,       Type,            Extent, Mip, Aniso, Filter, Mip Filter,                     "
           "Format,     Image Object, Views,                           Name, TQL info");
  }

  void endVisit()
  {
    eastl::sort(eastl::begin(textures), eastl::end(textures), [](auto &l, auto &r) {
      if (l->ressize() > r->ressize())
      {
        return true;
      }
      if (l->ressize() < r->ressize())
      {
        return false;
      }

      auto ln = l->getResName();
      auto rn = r->getResName();
      if (ln && rn)
      {
        return strcmp(ln, rn) < 0;
      }
      if (ln)
      {
        return true;
      }
      if (rn)
      {
        return false;
      }
      return l < r;
    });

    for (auto tex : textures)
    {
      auto ql = tql::get_tex_cur_ql(tex->getTID());
      auto extraInfo = tql::get_tex_info(tex->getTID(), false, tmpStor);
      auto img = tex->getDeviceImage();
      bool isAliased = false;
      size_t viewCount = 0;
      ByteUnits imageSize;
      if (img)
      {
        isAliased = img->isAliased();
        viewCount = img->getViewCount();
        if (!isAliased)
        {
          imageSize = img->getMemory().size();
        }
      }
      const char *typeName = "?";
      switch (tex->resType)
      {
        case RES3D_TEX: typeName = "2d"; break;
        case RES3D_CUBETEX: typeName = "cube"; break;
        case RES3D_VOLTEX: typeName = "vol"; break;
        case RES3D_ARRTEX: typeName = "array"; break;
        case RES3D_CUBEARRTEX: typeName = "cube array"; break;
      }
      if (!isAliased)
      {
        switch (tex->resType)
        {
          case RES3D_TEX:
            ++tex2DCount;
            tex2DSize += imageSize.value();
            break;
          case RES3D_CUBETEX:
            ++texCubeCount;
            texCubeSize += imageSize.value();
            break;
          case RES3D_VOLTEX:
            ++texVolCount;
            texVolSize += imageSize.value();
            break;
          case RES3D_ARRTEX:
            ++texArrayCount;
            texArraySize += imageSize.value();
            break;
          case RES3D_CUBEARRTEX:
            // currently counts as array tex
            ++texArrayCount;
            texArraySize += imageSize.value();
            break;
        }
      }
      else
      {
        ++aliasedCount;
      }
#define ALIASED_SIZE   "%12llu,        Aliased"
#define NORMAL_SIZE    "%12llu, %6.2f %7s"
#define TEXTURE_FIRST  ", %2d, %10s"
#define TEXTURE_SECOND ", %3d, %5d, %6s, %10s, %26s, %016p, %5d, %30s, %s"
      if (RES3D_TEX == tex->resType || RES3D_CUBETEX == tex->resType)
      {
        if (isAliased)
        {
          target(ALIASED_SIZE TEXTURE_FIRST ", %8dx%-8d" TEXTURE_SECOND, tex->ressize(), ql, typeName, tex->width, tex->height,
            tex->level_count(), tex->samplerState.getAniso(), skip_chars(filter_type_to_string(tex->samplerState.getFilter()), 18),
            skip_chars(filter_type_to_string(tex->samplerState.getMip()), 18), skip_chars(tex->getFormat().getNameString(), 12), img,
            viewCount, tex->getResName(), extraInfo);
        }
        else
        {
          target(NORMAL_SIZE TEXTURE_FIRST ", %8dx%-8d" TEXTURE_SECOND, tex->ressize(), imageSize.units(), imageSize.name(), ql,
            typeName, tex->width, tex->height, tex->level_count(), tex->samplerState.getAniso(),
            skip_chars(filter_type_to_string(tex->samplerState.getFilter()), 18),
            skip_chars(filter_type_to_string(tex->samplerState.getMip()), 18), skip_chars(tex->getFormat().getNameString(), 12), img,
            viewCount, tex->getResName(), extraInfo);
        }
      }
      else // if (RES3D_VOLTEX == tex->resType || RES3D_ARRTEX == tex->resType || RES3D_CUBEARRTEX == tex->resType)
      {
        if (isAliased)
        {
          target(ALIASED_SIZE TEXTURE_FIRST ", %5dx%-5dx%-5d" TEXTURE_SECOND, tex->ressize(), ql, typeName, tex->width, tex->height,
            tex->depth, tex->level_count(), tex->samplerState.getAniso(),
            skip_chars(filter_type_to_string(tex->samplerState.getFilter()), 18),
            skip_chars(filter_type_to_string(tex->samplerState.getMip()), 18), skip_chars(tex->getFormat().getNameString(), 12), img,
            viewCount, tex->getResName(), extraInfo);
        }
        else
        {
          target(NORMAL_SIZE TEXTURE_FIRST ", %5dx%-5dx%-5d" TEXTURE_SECOND, tex->ressize(), imageSize.units(), imageSize.name(), ql,
            typeName, tex->width, tex->height, tex->depth, tex->level_count(), tex->samplerState.getAniso(),
            skip_chars(filter_type_to_string(tex->samplerState.getFilter()), 18),
            skip_chars(filter_type_to_string(tex->samplerState.getMip()), 18), skip_chars(tex->getFormat().getNameString(), 12), img,
            viewCount, tex->getResName(), extraInfo);
        }
      }
#undef ALIASED_SIZE
#undef NORMAL_SIZE
#undef TEXTURE_FIRST
#undef TEXTURE_SECOND
    }

    // currently not counting aliasing textures, as they do not directly contribute to memory usage
    auto totalCount = tex2DCount + texCubeCount + texVolCount + texArrayCount;
    auto totalMem = tex2DSize + texCubeSize + texVolSize + texArraySize;
    target("Total: Texture %6.2f %7s over %u textures, 2D %6.2f %7s over %u, Cube %6.2f %7s "
           "over %u, Vol %6.2f %7s over %u, Array %6.2f %7s over %u, %u aliased textures",
      totalMem.units(), totalMem.name(), totalCount, tex2DSize.units(), tex2DSize.name(), tex2DCount, texCubeSize.units(),
      texCubeSize.name(), texCubeCount, texVolSize.units(), texVolSize.name(), texVolCount, texArraySize.units(), texArraySize.name(),
      texArrayCount, aliasedCount);
  }

  void visitTexture(const BaseTex *tex) { textures.push_back(tex); }
};

struct BaseTexDumpVisitor
{
  Tab<ResourceDumpInfo> &dumpInfo;
  eastl::vector_map<Image *, HeapDescPair> &imgToAddress;

  void beginVisit() {}
  void endVisit() {}
  void visitTexture(const drv3d_dx12::BaseTex *tex)
  {
    resource_dump_types::TextureTypes type = resource_dump_types::TextureTypes::TEX2D;
    switch (tex->resType)
    {
      case RES3D_TEX: type = resource_dump_types::TextureTypes::TEX2D; break;
      case RES3D_CUBETEX: type = resource_dump_types::TextureTypes::CUBETEX; break;
      case RES3D_VOLTEX: type = resource_dump_types::TextureTypes::VOLTEX; break;
      case RES3D_ARRTEX: type = resource_dump_types::TextureTypes::ARRTEX; break;
      case RES3D_CUBEARRTEX: type = resource_dump_types::TextureTypes::CUBEARRTEX; break;
    }
    uint64_t heapOffset = (uint64_t)-1;
    uint64_t heapId = (uint64_t)-1;
    Image *img = tex->getDeviceImage();
    if (img)
    {
      if (auto it = imgToAddress.find(img); it != imgToAddress.end())
      {
        heapOffset = it->second.heapOffset;
        heapId = (uint64_t)it->second.heapID;
      }
    }

    dumpInfo.emplace_back(ResourceDumpTexture({(uint64_t)-1, heapId, heapOffset, tex->width, tex->height, tex->level_count(),
      (tex->resType == RES3D_VOLTEX || tex->resType == RES3D_CUBEARRTEX) ? tex->depth : -1,
      (tex->resType == RES3D_TEX || tex->resType == RES3D_VOLTEX) ? -1 : (img ? img->getArrayLayers().count() : -1), tex->ressize(),
      tex->cflg, tex->getFormat().asTexFlags(), type, tex->getFormat().isColor(), tex->getResName(), ""}));
  }
};

template <typename T>
struct AliasingHeapReportVisitor
{
  T &target;
  uint32_t heapCount = 0;
  uint32_t bufferCount = 0;
  uint32_t textureCount = 0;
  ByteUnits totalSize;

  void beginVisit() { target("~ Aliasing / User Resource Heaps ~~~~"); }
  void endVisit()
  {
    target("%u Aliasing / User resource heaps, with %6.2f %7s, with %u texture, %u buffers", heapCount, totalSize.units(),
      totalSize.name(), textureCount, bufferCount);
  }
  void visitAliasingHeap(uint32_t id, const ResourceMemory &memory, const dag::Vector<Image *> &images,
    const dag::Vector<BufferGlobalId> &buffers, bool driver_managed)
  {
    ++heapCount;
    bufferCount += buffers.size();
    textureCount += images.size();
    totalSize += memory.size();
    ByteUnits size = memory.size();
    target("Aliasing Heap #%u with %6.2f %7s, %u textures, %u buffers%s", id, size.units(), size.name(), images.size(), buffers.size(),
      driver_managed ? " driver managed" : "");
    for (auto image : images)
    {
      image->getDebugName([this, image](auto name) { target("Texture %016p <%s>", image, name); });
    }
    for (auto buffer : buffers)
    {
      target("Buffer #%u", buffer.index());
    }
  }
};

const char *to_string(HostDeviceSharedMemoryRegion::Source src)
{
  switch (src)
  {
    default: return "<UNKNOWN>";
    case HostDeviceSharedMemoryRegion::Source::TEMPORARY: return "temporary";
    case HostDeviceSharedMemoryRegion::Source::PERSISTENT_UPLOAD: return "upload";
    case HostDeviceSharedMemoryRegion::Source::PERSISTENT_READ_BACK: return "read back";
    case HostDeviceSharedMemoryRegion::Source::PERSISTENT_BIDIRECTIONAL: return "bidirectional";
    case HostDeviceSharedMemoryRegion::Source::PUSH_RING: return "push ring";
  }
}

template <typename T>
struct FrameCompletionReportVisitor
{
  T &target;

  void beginVisit() { target("~ Pending resource frees ~~~~~~~~~~~~"); }
  void endVisit() {}
  void visitFrameCompletionData(uint32_t index, uint32_t total, const ResourceMemoryHeap::PendingForCompletedFrameData &data)
  {
    target("~ Data Set %u / %u ~~~~~~~~~~~~~~~~", 1 + index, total);
    bool wroteAnything = false;

    if (!data.deletedResourceHeaps.empty())
    {
      wroteAnything = true;
      target("User resource heap frees");
      for (auto heap : data.deletedResourceHeaps)
      {
        target("#%llu", reinterpret_cast<uintptr_t>(heap) - 1);
      }
    }

    if (!data.uploadBufferRefs.empty())
    {
      wroteAnything = true;
      target("Upload buffer refs");
      for (auto ref : data.uploadBufferRefs)
      {
        target("%016p", ref);
      }
      ByteUnits uploadUsage = data.uploadBufferUsage;
      ByteUnits tempUsage = data.tempUsage;
      target("Upload usage %6.2f %7s", uploadUsage.units(), uploadUsage.name());
      target("Temp usage %6.2f %7s", tempUsage.units(), tempUsage.name());
    }

    if (!data.uploadMemoryFrees.empty())
    {
      wroteAnything = true;
      target("Upload memory frees");
      for (auto &mem : data.uploadMemoryFrees)
      {
        ByteUnits size = mem.range.size();
        target("%016p, %016llX, %6.2f %7s, %016p, %s", mem.buffer, mem.range.front(), size.units(), size.name(), mem.pointer,
          to_string(mem.source));
      }
    }

    if (!data.readBackFrees.empty())
    {
      wroteAnything = true;
      target("Upload read back frees");
      for (auto &mem : data.readBackFrees)
      {
        ByteUnits size = mem.range.size();
        target("%016p, %016llX, %6.2f %7s, %016p, %s", mem.buffer, mem.range.front(), size.units(), size.name(), mem.pointer,
          to_string(mem.source));
      }
    }

    if (!data.bidirectionalFrees.empty())
    {
      wroteAnything = true;
      target("Upload bidirectional frees");
      for (auto &mem : data.bidirectionalFrees)
      {
        ByteUnits size = mem.range.size();
        target("%016p, %016llX, %6.2f %7s, %016p, %s", mem.buffer, mem.range.front(), size.units(), size.name(), mem.pointer,
          to_string(mem.source));
      }
    }

#if D3D_HAS_RAY_TRACING
    if (!data.deletedBottomAccelerationStructure.empty())
    {
      wroteAnything = true;
      target("Ray trace bottom acceleration structure freed");
      for (auto as : data.deletedBottomAccelerationStructure)
      {
        ByteUnits size = as->size;
        target("%016p, %6.2f %7s", as->asHeapResource, size.units(), size.name());
      }
    }

    if (!data.deletedTopAccelerationStructure.empty())
    {
      wroteAnything = true;
      target("Ray trace top acceleration structure freed");
      for (auto as : data.deletedTopAccelerationStructure)
      {
        ByteUnits size = as->size;
        target("%016p, %6.2f %7s", as->asHeapResource, size.units(), size.name());
      }
    }
#endif

    if (!data.deletedScratchBuffers.empty())
    {
      wroteAnything = true;
      target("Scratch buffer freed");
      for (auto &buf : data.deletedScratchBuffers)
      {
        ByteUnits size = buf.bufferMemory.size();
        target("%016p, %6.2f %7s", buf.buffer.Get(), size.units(), size.name());
      }
    }

    if (!data.freedTextureObjects.empty())
    {
      wroteAnything = true;
      target("Texture objects freed");
      for (auto tex : data.freedTextureObjects)
      {
        target("%s", tex->getTexName());
      }
    }

    if (!data.deletedImages.empty())
    {
      wroteAnything = true;
      target("Pending texture frees");
      target("          Size,          Address, Name");
      for (auto image : data.deletedImages)
      {
        if (image->isAliased())
        {
          image->getDebugName([this, image](auto &name) { target("%14s, %016p, %s", "Aliased", image, name); });
        }
        else
        {
          ByteUnits size = image->getMemory().size();
          image->getDebugName(
            [this, image, size](auto &name) { target("%6.2f %7s, %016p, %s", size.units(), size.name(), image, name); });
        }
      }
    }

    if (!data.deletedBuffers.empty())
    {
      wroteAnything = true;
      target("Pending buffer frees");
      target(" ID,           Offset,           Size, Free reason");
      for (auto &buf : data.deletedBuffers)
      {
        const char *freeReason = "??";
        switch (buf.freeReason)
        {
          case ResourceMemoryHeap::FreeReason::USER_DELETE: freeReason = "user delete"; break;
          case ResourceMemoryHeap::FreeReason::DISCARD_SAME_FRAME: freeReason = "discard (same frame)"; break;
          case ResourceMemoryHeap::FreeReason::DISCARD_DIFFERENT_FRAME: freeReason = "discard (different frame)"; break;
          case ResourceMemoryHeap::FreeReason::MOVED_BY_DRIVER: freeReason = "driver moved resource"; break;
        }
        ByteUnits size = buf.buffer.totalSize();
        target("%3u, %016llX, %6.2f %7s, %s", buf.buffer.resourceId.index(), buf.buffer.offset, size.units(), size.name(), freeReason);
      }
    }

    if (!wroteAnything)
    {
      target("Nothing is pending to be freed");
    }
  }
};

struct ResourceHeapWalker
{
  uint32_t tex2DCount = 0;
  uint32_t texCubeCount = 0;
  uint32_t texVolCount = 0;
  uint32_t texArrayCount = 0;
  uint32_t aliasedCount = 0;
  ByteUnits tex2DSize;
  ByteUnits texCubeSize;
  ByteUnits texVolSize;
  ByteUnits texArraySize;
  template <typename T>
  void operator()(DXGIAdapter *adapter, ResourceMemoryHeap &heap, T target)
  {
    target("~ OS reported memory usage ~~~~~~~~~~~");
#if _TARGET_XBOX
    G_UNUSED(adapter);
    size_t gameLimit = 0, gameUsed = 0;
    xbox_get_memory_status(gameUsed, gameLimit);
    ByteUnits gameLimitBytes = gameLimit;
    ByteUnits gameUsedBytes = gameUsed;
    target("%6.2f %7s (%zu Bytes) of %6.2f %7s (%zu Bytes) used", gameUsedBytes.units(), gameUsedBytes.name(), gameUsed,
      gameLimitBytes.units(), gameLimitBytes.name(), gameLimit);
#else
    if (adapter)
    {
      target("Location,                      Type,           Size,      Size In Bytes");
      const char *locName[] = {"GPU", "System"};
      DXGI_MEMORY_SEGMENT_GROUP group[] = {DXGI_MEMORY_SEGMENT_GROUP_LOCAL, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL};
      G_STATIC_ASSERT(countof(locName) == countof(group));
      for (uint32_t i = 0; i < countof(locName); ++i)
      {
        DXGI_QUERY_VIDEO_MEMORY_INFO info{};
        adapter->QueryVideoMemoryInfo(0, group[i], &info);
        ByteUnits budget = info.Budget;
        ByteUnits currentUsage = info.CurrentUsage;
        ByteUnits availableForReservation = info.AvailableForReservation;
        ByteUnits currentReservation = info.CurrentReservation;
        auto name = locName[i];
        target("%8s,                    Budget, %6.2f %7s, %12zu Bytes", name, budget.units(), budget.name(), info.Budget);
        target("%8s,             Current usage, %6.2f %7s, %12zu Bytes", name, currentUsage.units(), currentUsage.name(),
          info.CurrentUsage);
        target("%8s, Available for reservation, %6.2f %7s, %12zu Bytes", name, availableForReservation.units(),
          availableForReservation.name(), info.AvailableForReservation);
        target("%8s,       Current reservation, %6.2f %7s, %12zu Bytes", name, currentReservation.units(), currentReservation.name(),
          info.CurrentReservation);
      }
    }
    else
    {
      target("Adapter object was null, can not request system memory info");
    }
#endif

    heap.visitHeaps(ResourceHeapReportVisitor<T>{target});
    heap.visitAliasingHeaps(AliasingHeapReportVisitor<T>{target});

    ByteUnits pushRingSize = heap.getFramePushRingMemorySize();
    ByteUnits uploadRingSize = heap.getUploadRingMemorySize();
    ByteUnits tempUploadSize = heap.getTemporaryUploadMemorySize();
    ByteUnits persistUploadSize = heap.getPersistentUploadMemorySize();
    ByteUnits persistReadBackSize = heap.getPersistentReadBackMemorySize();
    ByteUnits persistBidirSize = heap.getPersistentBidirectionalMemorySize();

    target("~ Driver internal buffers ~~~~~~~~~~~");
    target("%6.2f %7s push ring buffers", pushRingSize.units(), pushRingSize.name());
    target("%6.2f %7s upload ring buffers", uploadRingSize.units(), uploadRingSize.name());
    target("%6.2f %7s temporary upload buffers", tempUploadSize.units(), tempUploadSize.name());
    target("%6.2f %7s persistent upload buffers", persistUploadSize.units(), persistUploadSize.name());
    target("%6.2f %7s persistent read back buffers", persistReadBackSize.units(), persistReadBackSize.name());
    target("%6.2f %7s persistent bidirectional buffers", persistBidirSize.units(), persistBidirSize.name());
    ByteUnits totalInternalSize = pushRingSize.value() + uploadRingSize.value() + tempUploadSize.value() + persistUploadSize.value() +
                                  persistReadBackSize.value() + persistBidirSize.value();
    target("%6.2f %7s total internal buffer sizes", totalInternalSize.units(), totalInternalSize.name());

    heap.visitBuffers(BufferHeapReportVisitor<T>{target});
    heap.visitBufferObjectsExplicit(SbufferReportVisitor<T>{target});
    heap.visitImageObjectsExplicit(ImageReportVisitor<T>{target});
    heap.visitTextureObjectsExplicit(BaseTexReportVisitor<T>{
      target, tex2DCount, texCubeCount, texVolCount, texArrayCount, aliasedCount, tex2DSize, texCubeSize, texVolSize, texArraySize});
    heap.visitFrameCompletionData(FrameCompletionReportVisitor<T>{target});
  }
};

struct ToDebugWriter
{
  template <typename... Ts>
  void operator()(Ts &&...ts)
  {
    logdbg(eastl::forward<Ts>(ts)...);
  }
};

struct ToStringWriter
{
  String &out;
  template <typename... Ts>
  void operator()(Ts &&...ts)
  {
    out.aprintf(128, eastl::forward<Ts>(ts)...);
    out.append("\n");
  }
};

struct ToNullWriter
{
  template <typename... Ts>
  void operator()(Ts &&...)
  {}
};
} // namespace

void ResourceMemoryHeap::generateResourceAndMemoryReport(DXGIAdapter *adapter, uint32_t *num_textures, uint64_t *total_mem,
  String *out_text)
{
  ResourceHeapWalker walker;
  if (out_text)
  {
    walker(adapter, *this, ToStringWriter{*out_text});
  }
  else
  {
    walker(adapter, *this, ToNullWriter{});
  }
  if (num_textures)
  {
    *num_textures = walker.tex2DCount + walker.texCubeCount + walker.texVolCount + walker.texArrayCount;
  }
  if (total_mem)
  {
    *total_mem = (walker.tex2DSize + walker.texCubeSize + walker.texVolSize + walker.texArraySize).value();
  }
}
void ResourceMemoryHeap::fillResourceDump(Tab<ResourceDumpInfo> &dump_container)
{
  eastl::vector_map<uint32_t, HeapDescPair> buffersPresent;
  eastl::vector_map<Image *, HeapDescPair> imgToAddress;
  visitHeaps(ResourceHeapDumpVisitor{dump_container, imgToAddress, buffersPresent});
  visitTextureObjectsExplicit(BaseTexDumpVisitor{dump_container, imgToAddress});
  visitBufferObjectsExplicit(SbufferDumpVisitor{dump_container, buffersPresent});
}


void OutOfMemoryRepoter::reportOOMInformation(DXGIAdapter *adapter)
{
  if (!didReportOOM)
  {
    didReportOOM = true;
    // cast assumes that OutOfMemoryRepoter is a base of ResourceMemoryHeap
    G_STATIC_ASSERT((eastl::is_base_of<OutOfMemoryRepoter, ResourceMemoryHeap>::value));
    // can't use static cast as it respects visibility rules
    logdbg("GPU MEMORY STATISTICS:\n======================");
    ResourceHeapWalker walker;
    walker(adapter, *(ResourceMemoryHeap *)this, ToDebugWriter{});
    logdbg("======================\nEND GPU MEMORY STATISTICS");
  }
  else
  {
    logdbg("GPU MEMORY STATISTICS: not reported as this is not the first time.");
  }
}

bool OutOfMemoryRepoter::checkForOOM(DXGIAdapter *adapter, bool was_okay, const OomReportData &report_data)
{
  if (!was_okay)
  {
    if (!get_device().isHealthyOrRecovering())
    {
      logwarn(
        "DX12: OOM check has failed, but device is not healthy or recovering. Skipping OOM report, we are going to reset device...");
      return was_okay;
    }
    D3D_ERROR("DX12: OOM report: %s", report_data.toString());
    reportOOMInformation(adapter);
    G_ASSERT_FAIL("DX12: OOM during %s (check logs to get more info)", report_data.getMethodName());
  }
  return was_okay;
}

HeapFragmentationManager::ResourceMoveResolution HeapFragmentationManager::moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter,
  ID3D12Device *device, frontend::BindlessManager &bindless_manager, HeapID heap_id, BufferGlobalId buffer_id,
  AllocationFlags allocation_flags, bool is_emergency_defragmentation)
{
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock bindlessStateLock{get_resource_binding_guard()};
  auto bufferHeapStateAccess = bufferHeapState.access();
  auto &bufferHeaps = bufferHeapStateAccess->bufferHeaps;
  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Trying to move buffer %u from %u:%u", buffer_id.index(),
    bufferHeaps[buffer_id.index()].bufferMemory.getHeapID().group, bufferHeaps[buffer_id.index()].bufferMemory.getHeapID().index);
  if (bufferHeaps[buffer_id.index()].getCPUPointer() != nullptr && !is_emergency_defragmentation)
  {
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to move buffer %u, buffer is mapable", buffer_id.index());
    return ResourceMoveResolution::STAYING;
  }
  auto movedBuffer = tryCloneBuffer(adapter, device, buffer_id, bufferHeapStateAccess, allocation_flags);
  if (!movedBuffer)
  {
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to move buffer %u, no space", buffer_id.index());
    // If we can't clone the buffer, then there is no space
    return ResourceMoveResolution::NO_SPACE;
  }
  if (moveStandbyBufferEntries(buffer_id, movedBuffer, bufferHeapStateAccess))
  {
    onMoveStandbyBufferEntriesSuccess(bufferHeaps, bufferHeapStateAccess, buffer_id, movedBuffer, heap_id,
      is_emergency_defragmentation);
    return ResourceMoveResolution::MOVED;
  }

  bool areDeviceObjectsMoved = false;
  {
    OSSpinlockUniqueLock lock{bufferPoolGuard, OSSpinlockUniqueLock::TryToLock{}};
    if (!lock)
    {
      // we fail to lock the buffer list we have to revert everything we done so far, otherwise we risk
      // having dangling stuff
      moveStandbyBufferEntries(movedBuffer, buffer_id, bufferHeapStateAccess);
      // fall through to clean up and tell the caller something happened
      DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: bufferPoolGuard lock failed");
    }
    else
    {
      findBufferOwnersAndReplaceDeviceObjects(buffer_id, movedBuffer, bindless_manager, ctx, device, bufferHeapStateAccess,
        is_emergency_defragmentation);
      areDeviceObjectsMoved = true;
    }
  }

  tidyCloneBuffer(buffer_id, bufferHeapStateAccess);
  // Always tidy up after we are done
  const bool isBufferDeleted = tidyCloneBuffer(movedBuffer, bufferHeapStateAccess);
  if (!areDeviceObjectsMoved)
    return ResourceMoveResolution::STAYING;
  if (isBufferDeleted)
  {
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Buffer %u deleted", buffer_id.index());
    return ResourceMoveResolution::DELETED;
  }
  return ResourceMoveResolution::MOVING;
}

void HeapFragmentationManager::onMoveStandbyBufferEntriesSuccess(const dag::Vector<Heap> &buffer_heaps,
  BufferHeapStateWrapper::AccessToken &buffer_heap_state_access, BufferGlobalId old_buffer, BufferGlobalId moved_buffer,
  HeapID heap_id, bool is_emergency_defragmentation)
{
  auto &oldBuffer = buffer_heaps[old_buffer.index()];
  auto &newBuffer = buffer_heaps[moved_buffer.index()];
  G_UNUSED(oldBuffer);
  G_UNUSED(newBuffer);
  G_UNUSED(heap_id);

  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Moved standby buffer %u from %u:%u to %u:%u", old_buffer.index(),
    oldBuffer.bufferMemory.getHeapID().group, oldBuffer.bufferMemory.getHeapID().index, newBuffer.bufferMemory.getHeapID().group,
    newBuffer.bufferMemory.getHeapID().index, heap_id.group, heap_id.index);

  // If the current buffer was only referenced by the standby list we have to tidy it instead of the moved buffer.
  tidyCloneBuffer(old_buffer, buffer_heap_state_access);
}

void HeapFragmentationManager::moveBufferDeviceObject(GenericBufferInterface *buffer_object, BufferGlobalId moved_buffer,
  frontend::BindlessManager &bindless_manager, DeviceContext &ctx, ID3D12Device *device,
  BufferHeapStateWrapper::AccessToken &buffer_heap_state_access, bool is_emergency_defragmentation)
{
  auto &currentBuffer = buffer_object->getDeviceBuffer();

  auto newBuffer = moveBuffer(currentBuffer, moved_buffer, buffer_heap_state_access);

  // Recreate needed views
  if (buffer_object->usableAsShaderResource())
  {
    G_ASSERT(currentBuffer.srvs);
    if (buffer_object->usesRawView())
      this->createBufferRawSRV(device, newBuffer);
    else if (buffer_object->usesStructuredView())
      this->createBufferStructureSRV(device, newBuffer, buffer_object->getElementSize());
    else
      this->createBufferTextureSRV(device, newBuffer, buffer_object->getTextureViewFormat());
  }
  if (buffer_object->usableAsUnorderedResource())
  {
    G_ASSERT(currentBuffer.uavs);
    if (buffer_object->usesRawView())
      this->createBufferRawUAV(device, newBuffer);
    else if (buffer_object->usesStructuredView())
      this->createBufferStructureUAV(device, newBuffer, buffer_object->getElementSize());
    else
      this->createBufferTextureUAV(device, newBuffer, buffer_object->getTextureViewFormat());
  }

  mark_buffer_stages_dirty_no_lock(buffer_object, buffer_object->usableAsVertexBuffer(), buffer_object->usableAsConstantBuffer(),
    buffer_object->usableAsShaderResource(), buffer_object->usableAsUnorderedResource());

  // Update bindless heap
  if (newBuffer.srvs)
    bindless_manager.updateBufferReferencesNoLock(ctx, currentBuffer.currentSRV(), newBuffer.currentSRV());

    // If we ever use bindless UAV then this has to be enabled.
#if 0
  if (newBuffer.uavs)
  {
    for (uint32_t i = 0; i < newBuffer.discardCount; ++i)
    {
      bindless_manager.onBufferMove(ctx, currentBuffer.uavs[i], newBuffer.uavs[i]);
    }
  }
#endif

  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Moved buffer %u to %u (%s)", currentBuffer.resourceId.index(),
    newBuffer.resourceId.index(), buffer_object->getResName());
  // copy contents from the old to the new buffer
  ctx.moveBuffer(currentBuffer, newBuffer);

  this->freeBufferOnFrameCompletion(eastl::move(currentBuffer), FreeReason::MOVED_BY_DRIVER);

  currentBuffer = eastl::move(newBuffer);
}

void HeapFragmentationManager::findBufferOwnersAndReplaceDeviceObjects(BufferGlobalId old_buffer, BufferGlobalId moved_buffer,
  frontend::BindlessManager &bindless_manager, DeviceContext &ctx, ID3D12Device *device,
  BufferHeapStateWrapper::AccessToken &buffer_heap_state_access, bool is_emergency_defragmentation)
{
  bool isUsed = false;
  bufferPool.iterateAllocatedBreakable(
    [=, &ctx, &bindless_manager, &buffer_heap_state_access, &isUsed](GenericBufferInterface *buffer_object) { //-V657
      auto &currentBuffer = buffer_object->getDeviceBuffer();
      if (currentBuffer.resourceId != old_buffer)
      {
        // keep going
        return true;
      }
      isUsed = true;

      moveBufferDeviceObject(buffer_object, moved_buffer, bindless_manager, ctx, device, buffer_heap_state_access,
        is_emergency_defragmentation);

      // keep looking for possible users of this buffer
      // TODO we can be smart about it and stop if sub alloc is off or unsupported.
      return true;
    });
  if (isUsed)
    return;
  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Owner of buffer %u has not been found", old_buffer.index());
}

HeapFragmentationManager::ResourceMoveResolution HeapFragmentationManager::moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter,
  ID3D12Device *device, frontend::BindlessManager &bindless_manager, HeapID heap_id, Image *texture, AllocationFlags allocation_flags,
  bool is_emergency_defragmentation)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock resourceBindingLock(get_resource_binding_guard());

  G_UNUSED(heap_id);
  BaseTex *baseTex = nullptr;

  if (texture->isAliased())
  {
    texture->getDebugName([=](const auto &name) {
      G_UNUSED(name);
      DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to move texture <%s>, texture aliased", name.c_str());
    });
    return ResourceMoveResolution::STAYING;
  }

  {
    auto texturePoolAccess = texturePool.access();

    texturePoolAccess->iterateAllocatedBreakable([texture, &baseTex](auto *tex) //
      {
        if (texture == tex->getDeviceImage())
        {
          baseTex = tex;
          return false;
        }
        return true;
      });
  }
  if (!baseTex)
  {
    texture->getDebugName([=](const auto &name) {
      G_UNUSED(name);
      DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to move texture <%s>, no BaseTex found", name.c_str());
    });
    return ResourceMoveResolution::STAYING;
  }
#if _TARGET_XBOX
  if (baseTex->cflg & TEXCF_LINEAR_LAYOUT && !is_emergency_defragmentation)
  {
    texture->getDebugName([=](const auto &name) {
      G_UNUSED(name);
      DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to move texture <%s>, texture has linear layout", name.c_str());
    });
    return ResourceMoveResolution::STAYING;
  }
#endif
  // 1) For base stub textures (see tql::initTexStubs) we don't have a valid stub index. So we have to look for them in the array
  //    (or add index?)
  // 2) TODO: handle stubs correctly before enabling this
  if (baseTex->isStub() || tql::isTexStub(baseTex))
  {
    texture->getDebugName([=](const auto &name) {
      G_UNUSED(name);
      DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to move texture <%s>, texture is a stub", name.c_str());
    });
    return ResourceMoveResolution::STAYING;
  }
  if (baseTex->isLocked())
  {
    texture->getDebugName([=](const auto &name) {
      G_UNUSED(name);
      DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to move texture <%s>, texture locked", name.c_str());
    });
    return ResourceMoveResolution::STAYING;
  }
  static constexpr uint32_t delayFromCreation = 4;
  if (ctx.getRecordingFenceProgress() - baseTex->getCreationFenceProgress() <= delayFromCreation)
  {
    texture->getDebugName([=](const auto &name) {
      G_UNUSED(name);
      DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to move texture <%s>, texture created few frames ago", name.c_str());
    });
    return ResourceMoveResolution::STAYING;
  }
  const auto bindlessInfo = bindless_manager.checkTextureImagePairNoLock(baseTex, texture);
  if (bindlessInfo.mismatchFound || baseTex->memSize == 0)
  {
    // If the bindless heap has a entry for baseTex but with a different texture, then baseTex is updated to use
    // texture but the bindless heap was not updated yet. To avoid problems we back out and try again later.
    texture->getDebugName([=](const auto &name) {
      G_UNUSED(name);
      DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to move texture <%s>, texture internals are updating", name.c_str());
    });
    return ResourceMoveResolution::STAYING;
  }
  texture->getDebugName([=](const auto &name) {
    G_UNUSED(name);
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Trying to move texture <%s>", name.c_str());
  });
  const D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET;
  auto newTexture = tryCloneTexture(adapter, device, texture, initialState, allocation_flags);
  if (!newTexture)
  {
    texture->getDebugName([=](const auto &name) {
      G_UNUSED(name);
      DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to move texture <%s>, no space", name.c_str());
    });
    // Clone failed, only realistic chance is no space is available
    return ResourceMoveResolution::NO_SPACE;
  }
  ctx.setImageResourceStateNoLock(initialState, newTexture->getGlobalSubresourceIdRange());
  if (!baseTex->swapTextureNoLock(texture, newTexture))
  {
    texture->getDebugName([=](const auto &name) {
      G_UNUSED(name);
      DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to move texture <%s>, swap failed", name.c_str());
    });
    destroyTextureOnFrameCompletion(newTexture);
    // have to indicate moving as some data structures may have changed
    return ResourceMoveResolution::MOVING;
  }
  if (!tryDestroyTextureOnFrameCompletion(texture))
  {
    texture->getDebugName([=](const auto &name) {
      G_UNUSED(name);
      DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Texture <%s> queued for deletion", name.c_str());
      destroyTextureOnFrameCompletion(newTexture);
    });
    return ResourceMoveResolution::QUEUED_FOR_DELETION;
  }
  if (bindlessInfo.matchCount > 0)
  {
    bindless_manager.updateTextureReferencesNoLock(ctx, baseTex, texture, bindlessInfo.firstFound, bindlessInfo.matchCount, false);
  }
  ctx.moveTextureNoLock(texture, newTexture);

  texture->getDebugName([=](const auto &name) {
    G_UNUSED(name);
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Moving texture <%s>, new location %u (%u:%u)", name.c_str(),
      newTexture->getMemory().getHeapID().raw, newTexture->getMemory().getHeapID().group, newTexture->getMemory().getHeapID().index);
  });
  return ResourceMoveResolution::MOVING;
}

HeapFragmentationManager::ResourceMoveResolution HeapFragmentationManager::moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter,
  ID3D12Device *device, frontend::BindlessManager &bindless_manager, HeapID heap_id, AliasHeapReference ref,
  AllocationFlags allocation_flags, bool is_emergency_defragmentation)
{
  G_UNUSED(ctx);
  G_UNUSED(adapter);
  G_UNUSED(device);
  G_UNUSED(bindless_manager);
  G_UNUSED(heap_id);
  G_UNUSED(ref);
  G_UNUSED(allocation_flags);
  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Trying to move alias heap (not implemented)");
  return ResourceMoveResolution::STAYING;
}

HeapFragmentationManager::ResourceMoveResolution HeapFragmentationManager::moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter,
  ID3D12Device *device, frontend::BindlessManager &bindless_manager, HeapID heap_id, ScratchBufferReference ref,
  AllocationFlags allocation_flags, bool is_emergency_defragmentation)
{
  G_UNUSED(ctx);
  G_UNUSED(bindless_manager);
  G_UNUSED(heap_id);

  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Trying to move scratch buffer");
  auto scratchBufferAccess = tempScratchBufferState.access();
  if (scratchBufferAccess->buffer.buffer.Get() != ref.buffer)
  {
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Failed to move scratch buffer, is not current one");
    // If we try to move a scratch buffer that is not the current one, then the one we want to move
    // is probably queued for deletion already
    return ResourceMoveResolution::QUEUED_FOR_DELETION;
  }
  if (tryMoveScratchBuffer(scratchBufferAccess, adapter, device, allocation_flags))
  {
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Moved scratch buffer");
    return ResourceMoveResolution::MOVING;
  }
  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to move scratch buffer");
  // Only way move can fail is if there is no space in non locked free memory
  return ResourceMoveResolution::NO_SPACE;
}

HeapFragmentationManager::ResourceMoveResolution HeapFragmentationManager::moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter,
  ID3D12Device *device, frontend::BindlessManager &bindless_manager, HeapID heap_id, PushRingBufferReference ref,
  AllocationFlags allocation_flags, bool is_emergency_defragmentation)
{
  G_UNUSED(ctx);
  G_UNUSED(bindless_manager);
  G_UNUSED(heap_id);

  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Trying to move push ring buffer");
  if (onMovePushRingBuffer(adapter, device, ref.buffer, allocation_flags))
  {
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Moved push ring buffer");
    return ResourceMoveResolution::MOVING;
  }
  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to move push ring buffer");
  // Only way it can fail is if we ran out of memory for a replacement segment
  return ResourceMoveResolution::NO_SPACE;
}

HeapFragmentationManager::ResourceMoveResolution HeapFragmentationManager::moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter,
  ID3D12Device *device, frontend::BindlessManager &bindless_manager, HeapID heap_id, UploadRingBufferReference ref,
  AllocationFlags allocation_flags, bool is_emergency_defragmentation)
{
  G_UNUSED(ctx);
  G_UNUSED(bindless_manager);
  G_UNUSED(heap_id);

  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Trying to move upload ring buffer");
  if (onMoveUploadRingBuffer(adapter, device, ref.buffer, allocation_flags))
  {
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Moved upload ring buffer");
    return ResourceMoveResolution::MOVING;
  }
  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to upload push ring buffer");
  // Only way it can fail is if we ran out of memory for a replacement segment
  return ResourceMoveResolution::NO_SPACE;
}

HeapFragmentationManager::ResourceMoveResolution HeapFragmentationManager::moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter,
  ID3D12Device *device, frontend::BindlessManager &bindless_manager, HeapID heap_id, TempUploadBufferReference ref,
  AllocationFlags allocation_flags, bool is_emergency_defragmentation)
{
  G_UNUSED(ctx);
  G_UNUSED(bindless_manager);
  G_UNUSED(heap_id);

  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Trying to move temp upload buffer");
  if (tryMoveTemporaryUploadStandbyBuffer(adapter, device, ref.buffer, allocation_flags))
  {
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Moved temporary upload buffer (was standby)");
    return ResourceMoveResolution::MOVED;
  }
  if (tryMoveTemporaryUploadBuffer(adapter, device, ref.buffer, allocation_flags))
  {
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Moved temporary upload buffer");
    return ResourceMoveResolution::MOVING;
  }
  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Unable to temporary upload buffer");
  // Only way it can fail is if we ran out of memory for a replacement segment
  return ResourceMoveResolution::NO_SPACE;
}

HeapFragmentationManager::ResourceMoveResolution HeapFragmentationManager::moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter,
  ID3D12Device *device, frontend::BindlessManager &bindless_manager, HeapID heap_id, PersistentUploadBufferReference ref,
  AllocationFlags allocation_flags, bool is_emergency_defragmentation)
{
  G_UNUSED(ctx);
  G_UNUSED(adapter);
  G_UNUSED(device);
  G_UNUSED(bindless_manager);
  G_UNUSED(heap_id);
  G_UNUSED(ref);
  G_UNUSED(allocation_flags);
  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Trying to move persistent upload buffer (not implemented)");
  return ResourceMoveResolution::STAYING;
}

HeapFragmentationManager::ResourceMoveResolution HeapFragmentationManager::moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter,
  ID3D12Device *device, frontend::BindlessManager &bindless_manager, HeapID heap_id, PersistentReadBackBufferReference ref,
  AllocationFlags allocation_flags, bool is_emergency_defragmentation)
{
  G_UNUSED(ctx);
  G_UNUSED(adapter);
  G_UNUSED(device);
  G_UNUSED(bindless_manager);
  G_UNUSED(heap_id);
  G_UNUSED(ref);
  G_UNUSED(allocation_flags);
  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Trying to move persistent read back buffer (not implemented)");
  return ResourceMoveResolution::STAYING;
}

HeapFragmentationManager::ResourceMoveResolution HeapFragmentationManager::moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter,
  ID3D12Device *device, frontend::BindlessManager &bindless_manager, HeapID heap_id, PersistentBidirectionalBufferReference ref,
  AllocationFlags allocation_flags, bool is_emergency_defragmentation)
{
  G_UNUSED(ctx);
  G_UNUSED(adapter);
  G_UNUSED(device);
  G_UNUSED(bindless_manager);
  G_UNUSED(heap_id);
  G_UNUSED(ref);
  G_UNUSED(allocation_flags);
  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Trying to move persistent bidirectional buffer (not implemented)");
  return ResourceMoveResolution::STAYING;
}

HeapFragmentationManager::ResourceMoveResolution HeapFragmentationManager::moveResourceAway(DeviceContext &ctx, DXGIAdapter *adapter,
  ID3D12Device *device, frontend::BindlessManager &bindless_manager, HeapID heap_id, RaytraceAccelerationStructureHeapReference ref,
  AllocationFlags allocation_flags, bool is_emergency_defragmentation)
{
  G_UNUSED(ctx);
  G_UNUSED(adapter);
  G_UNUSED(device);
  G_UNUSED(bindless_manager);
  G_UNUSED(heap_id);
  G_UNUSED(ref);
  G_UNUSED(allocation_flags);
  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Trying to move raytrace acceleration structure heap (not implemented)");
  return ResourceMoveResolution::STAYING;
}

bool HeapFragmentationManager::processGroup(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
  frontend::BindlessManager &bindless_manager, uint32_t index)
{
  auto &state = groupsFragmentationState[index];
  // Wait until all outstanding moves are complete, otherwise it can be very complex to avoid moving
  // just moved resources.
  if (state.activeMoves > 0)
    return false;

  if (getHeapGroupGeneration(index) == state.skipGeneration)
    return false;

  // The first strategy is to move all allocated memory / resources of one heap to all others,
  // to be able to free the heap. This aims to reduce the amount of memory we allocate from
  // the system. This strategy is not used when we have fixed heaps, as we can not free any
  // memory with this strategy.
  //
  // tryDispenseHeap(...); // we don't use it, because it seems to be ineffective

  // The second strategy looks for the largest free memory region of all the heaps and tries
  // to enlarges it, and so reducing fragmentation.
  const bool isDefragmentationInProgress = tryDefragmentHeap(ctx, adapter, device, bindless_manager, index, false);
  if (isDefragmentationInProgress)
    return true;

  state.lockedRanges.clear();
  state.skipGeneration = getHeapGroupGeneration(index);
  return false;
}

bool HeapFragmentationManager::tryDefragmentHeap(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
  frontend::BindlessManager &bindless_manager, uint32_t index, bool is_emergency_defragmentation)
{
  auto &group = groups[index];
  auto &state = groupsFragmentationState[index];

  OSSpinlockScopedLock heapLock{heapGroupMutex};

  setDefragmentationGeneration(index);

  if (group.empty())
    return false;

  if (getHeapGroupGeneration(index) != state.currentGeneration)
  {
    state.lockedRanges.clear();
    state.currentGeneration = getHeapGroupGeneration(index);
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: group: %u, new generation: %u, locked ranges reset", index,
      state.currentGeneration);
  }

  lockRangesBeforeDefragmentationStep(index);

  auto [selectedHeap, selectedRange] = find_heap_for_defragmentation(group);
  if (selectedHeap == group.end())
  {
    unlockRangesAfterDefragmentationStep(index);
    return false;
  }

  HeapID srcId;
  srcId.group = index;
  srcId.index = eastl::distance(begin(group), selectedHeap);

  const bool defragmentationStepResult =
    processDefragmentationStep(ctx, adapter, device, bindless_manager, srcId, *selectedRange, is_emergency_defragmentation);

  unlockRangesAfterDefragmentationStep(index);

  if (!checkDefragmentationGeneration(srcId.group))
  {
    state.lockedRanges.clear();
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Continue defragmentation because group has been changed (%u)", srcId.group);
    return true; // group has been changed -- continue defragmentation
  }

  if (!defragmentationStepResult && state.lockedRanges.max_size() > state.lockedRanges.size())
  {
    state.lockedRanges.push_back({srcId.raw, *selectedRange});
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Range locked (heap: %u (%u:%u), 0x%016X - 0x%016X)", srcId.raw, srcId.group,
      srcId.index, selectedRange->start, selectedRange->stop);
    return true; // range has been locked -- we can try with another
  }

  if (!defragmentationStepResult)
  {
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Max ranges count reached (group: %u, ranges count: %u)", index,
      state.lockedRanges.size());
    return false;
  }

  return true;
}

eastl::pair<HeapFragmentationManager::HeapIterator, HeapFragmentationManager::RangeIterator> HeapFragmentationManager::
  find_heap_for_defragmentation(dag::Vector<ResourceHeap> &heaps)
{
  // Find first heap that has free ranges.
  auto selectedHeap = eastl::find_if(begin(heaps), end(heaps),
    [](auto &heap) //
    { return static_cast<bool>(heap) && !heap.freeRanges.empty(); });
  if (selectedHeap == end(heaps))
    return {end(heaps), {}};

  auto getLargestFreeRange = [](ResourceHeap &heap) {
    auto &ranges = heap.freeRanges;
    const auto &lockedRange = heap.lockedRange;

    auto selected = eastl::find_if(begin(ranges), end(ranges),
      [=](const ValueRange<uint64_t> &candidate) { return !candidate.overlaps(lockedRange); });
    if (selected == end(ranges))
      return end(ranges);

    for (auto candidate = selected + 1; candidate != end(ranges); ++candidate)
    {
      if (candidate->size() <= selected->size())
        continue;
      if (candidate->overlaps(lockedRange))
        continue;
      selected = candidate;
    }

    return selected;
  };

  auto selectedRange = getLargestFreeRange(*selectedHeap);
  while (selectedRange == end(selectedHeap->freeRanges))
  {
    selectedHeap++;
    if (selectedHeap == end(heaps))
      return {end(heaps), {}};
    selectedRange = getLargestFreeRange(*selectedHeap);
  }
  bool multipleHeapsWithFreeRange = false;
  for (auto candidate = selectedHeap + 1; candidate != end(heaps); ++candidate)
  {
    if (!*candidate)
      continue;
    if (candidate->freeRanges.empty())
      continue;
    auto candidateRange = getLargestFreeRange(*candidate);
    if (candidateRange == end(candidate->freeRanges))
      continue;

    multipleHeapsWithFreeRange = true;

    if (candidateRange->size() < selectedRange->size())
      continue;
    if (candidateRange->size() == selectedRange->size())
    {
      // Tiebreaker, select the smaller one so we may be able to completely free everything it
      // holds and so can free the heap.
      if (candidate->totalSize > selectedHeap->totalSize)
        continue;
    }

    selectedHeap = candidate;
    selectedRange = candidateRange;
  }

  if (!multipleHeapsWithFreeRange && (1 == selectedHeap->freeRanges.size()))
  {
    // When there is only one heap with free ranges and it has only one range, then there is nothing
    // to do
    return {end(heaps), {}};
  }

  return {selectedHeap, selectedRange};
}

void HeapFragmentationManager::lockRangesBeforeDefragmentationStep(uint32_t group_index)
{
  auto &group = groups[group_index];
  auto &state = groupsFragmentationState[group_index];
  for (const auto &lockedRange : state.lockedRanges)
  {
    G_ASSERT(HeapID{lockedRange.first}.group == group_index);
    auto &heap = group[HeapID{lockedRange.first}.index];
    if (heap.isLocked())
    {
      heap.unlock();
      heap.lockFullRange();
    }
    else
      heap.lock(lockedRange.second);
  }
}

void HeapFragmentationManager::unlockRangesAfterDefragmentationStep(uint32_t group_index)
{
  auto &group = groups[group_index];
  for (auto &range : group)
    range.unlock();
}

bool HeapFragmentationManager::processDefragmentationStep(DeviceContext &ctx, DXGIAdapter *adapter, ID3D12Device *device,
  frontend::BindlessManager &bindless_manager, HeapID srcId, ValueRange<uint64_t> selectedRange, bool is_emergency_defragmentation)
  DAG_TS_REQUIRES(heapGroupMutex)
{
  DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Selected heap %u (%u:%u) for defragmentation (range: 0x%016X - 0x%016X)",
    srcId.raw, srcId.group, srcId.index, selectedRange.start, selectedRange.stop);

  auto &selectedHeap = groups[srcId.group][srcId.index];
  uint32_t successCount = 0;

  if (selectedHeap.isLocked())
  {
    selectedHeap.unlock();
    selectedHeap.lockFullRange();
  }
  else
    selectedHeap.lock(selectedRange);

  auto tryMoveRangeResource = [=, &ctx, &bindless_manager, &successCount](
                                ResourceHeap::UsedRangeSetType::iterator selectedRes) DAG_TS_REQUIRES(heapGroupMutex) {
    AllocationFlags allocFlags{AllocationFlag::DISALLOW_LOCKED_RANGES, AllocationFlag::DEFRAGMENTATION_OPERATION,
      AllocationFlag::EXISTING_HEAPS_ONLY, AllocationFlag::DISABLE_ALTERNATE_HEAPS};

    const auto resource = selectedRes->resource;
    heapGroupMutex.unlock();
    const auto moveResult = eastl::visit(
      [&ctx, this, srcId, adapter, device, &bindless_manager, allocFlags, is_emergency_defragmentation](auto &ref) {
        return moveResourceAway(ctx, adapter, device, bindless_manager, srcId, ref, allocFlags, is_emergency_defragmentation);
      },
      resource);
    heapGroupMutex.lock();
    if (moveResult == ResourceMoveResolution::MOVING)
    {
      groupsFragmentationState[srcId.group].activeMoves++;
      accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>([=](auto &data) { ++data.executedMoves[srcId.group]; });
    }

    if (moveResult != ResourceMoveResolution::NO_SPACE && moveResult != ResourceMoveResolution::STAYING)
      successCount++;
    else
    {
      DEFRAG_VERBOSE(is_emergency_defragmentation,
        "DX12: Defragmentation move failed -- heap %u (%u:%u) -- resource cannot be moved (offset: %u, status: %d)", srcId.raw,
        srcId.group, srcId.index, selectedRange.start, static_cast<eastl::underlying_type_t<ResourceMoveResolution>>(moveResult));
    }
  };

  const auto firstSelectedRes = eastl::upper_bound(begin(selectedHeap.usedRanges), end(selectedHeap.usedRanges), selectedRange.start,
    [](uint64_t start, const HeapResourceInfo &info) { return start < info.range.start; });

  if (firstSelectedRes != end(selectedHeap.usedRanges))
    tryMoveRangeResource(firstSelectedRes);

  if (successCount == 0 && checkDefragmentationGeneration(srcId.group)) // try another side
  {
    const auto reverseSecondSelectedRes = eastl::upper_bound(rbegin(selectedHeap.usedRanges), rend(selectedHeap.usedRanges),
      selectedRange.start, [](uint64_t start, const HeapResourceInfo &info) { return start > info.range.start; });

    if (reverseSecondSelectedRes != rend(selectedHeap.usedRanges))
    {
      const auto secondSelectedRes = reverseSecondSelectedRes.base() - 1;
      tryMoveRangeResource(secondSelectedRes);
    }
  }

  if (successCount == 0)
    DEFRAG_VERBOSE(is_emergency_defragmentation, "DX12: Defragmentation step failed -- all moves failed");
  else
    DEFRAG_VERBOSE(is_emergency_defragmentation,
      "DX12: Defragmentation step finished for heap %u (%u:%u) -- active moves: %u, total moves: %u", srcId.raw, srcId.group,
      srcId.index, groupsFragmentationState[srcId.group].activeMoves, successCount);

  return successCount > 0; // -V1020 we will unlock heap after moves processing (completeFrameExecution)
}

void HeapFragmentationManager::completeFrameRecording(Device &device, DXGIAdapter *adapter,
  frontend::BindlessManager &bindless_manager, uint32_t history_index)
{
  ResourceMemoryHeap::CompletedFrameRecordingInfo frameCompleteInfo;
#if _TARGET_PC_WIN && DX12_SUPPORT_RESOURCE_MEMORY_METRICS
  frameCompleteInfo.adapter = adapter;
#endif
  frameCompleteInfo.historyIndex = history_index;
  BaseType::completeFrameRecording(frameCompleteInfo);

  if (!device.isDefragmentationEnabled())
    return;

  for (auto &group : groups)
    for (auto &heap : group)
    {
      // Other locks will conflict with defragmentation
      // Here we also check that locked in previous defragmentation steps ranges was unlocked properly
      G_ASSERT(!heap.isLocked());
      G_UNUSED(heap);
    }

  for (uint32_t grp = 0; grp < groups.size(); ++grp)
  {
    const uint32_t currentGroup = (groupOffset + grp) % groups.size();
    const bool isDefragmentationInProgress =
      processGroup(device.getContext(), adapter, device.getDevice(), bindless_manager, currentGroup);
    if (isDefragmentationInProgress)
      break; // Defragmentation is heavy operation. We can process only one group per frame
  }
  groupOffset++;
}

void HeapFragmentationManager::processEmergencyDefragmentationForGroup(Device &device, DXGIAdapter *adapter,
  frontend::BindlessManager &bindless_manager, uint32_t group_index)
{
  logdbg("DX12: Processing emergency defragmentation for heap group: %d", group_index);
  // during emergency defragmentation we do not use locked ranges from previous steps
  groupsFragmentationState[group_index].lockedRanges.clear();
  tryDefragmentHeap(device.getContext(), adapter, device.getDevice(), bindless_manager, group_index, true);
}

void drv3d_dx12::resource_manager::HeapFragmentationManager::processEmergencyDefragmentation(Device &device, DXGIAdapter *adapter,
  frontend::BindlessManager &bindless_manager, uint32_t group_index, bool is_alternate_heap_allowed, bool is_uav, bool is_rtv)
{
  processEmergencyDefragmentationForGroup(device, adapter, bindless_manager, group_index);

  if (!is_alternate_heap_allowed)
    return;

  const ResourceHeapProperties properties = {group_index};

  for (ResourceHeapProperties currentProperties = {}; currentProperties.raw < groups.size(); currentProperties.raw++)
  {
    if (currentProperties.raw == group_index)
      continue;
    G_UNUSED(is_rtv);
#if _TARGET_XBOX
    const bool isCompatible = properties.isCompatible(currentProperties, is_uav, is_rtv);
#else
    const bool isCompatible = properties.isCompatible(currentProperties, canMixResources(), isUMASystem(), is_uav);
#endif
    if (!isCompatible)
      continue;
    processEmergencyDefragmentationForGroup(device, adapter, bindless_manager, currentProperties.raw);
  }
}

ScratchBuffer ScratchBufferProvider::getTempScratchBufferSpace(DXGIAdapter *adapter, Device &device, size_t size, size_t alignment)
{
  auto result = tempScratchBufferState.access()->getSpace(adapter, device.getDevice(), size, alignment, this);
  if (!result)
  {
    device.processEmergencyDefragmentation(getScratchBufferHeapProperties().raw, true, false, false);
    result = tempScratchBufferState.access()->getSpace(adapter, device.getDevice(), size, alignment, this);
  }
  checkForOOM(adapter, static_cast<bool>(result),
    OomReportData{"getTempScratchBufferSpace", nullptr, size, AllocationFlags{}.to_ulong(), getScratchBufferHeapProperties().raw});
  if (result)
  {
    recordScratchBufferTempUse(size);
  }
  return result;
}

ScratchBuffer ScratchBufferProvider::getPersistentScratchBufferSpace(DXGIAdapter *adapter, Device &device, size_t size,
  size_t alignment)
{
  auto result = tempScratchBufferState.access()->getPersistentSpace(adapter, device.getDevice(), size, alignment, this);
  if (!result)
  {
    device.processEmergencyDefragmentation(getScratchBufferHeapProperties().raw, true, false, false);
    result = tempScratchBufferState.access()->getPersistentSpace(adapter, device.getDevice(), size, alignment, this);
  }
  checkForOOM(adapter, static_cast<bool>(result),
    OomReportData{
      "getPersistentScratchBufferSpace", nullptr, size, AllocationFlags{}.to_ulong(), getScratchBufferHeapProperties().raw});
  if (result)
  {
    recordScratchBufferPersistentUse(size);
  }
  return result;
}
