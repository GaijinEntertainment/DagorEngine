#include "device.h"

#include <supp/dag_cpuControl.h>


using namespace drv3d_dx12;
using namespace drv3d_dx12::resource_manager;

ImageCreateResult TextureImageFactory::createTexture(DXGIAdapter *adapter, ID3D12Device *device, const ImageInfo &ii, const char *name)
{
  ImageCreateResult result{};
  auto desc = ii.asDesc();

  result.state = propertiesToInitialState(desc.Dimension, desc.Flags, ii.memoryClass);

  if (ii.memoryClass != DeviceMemoryClass::RESERVED_RESOURCE)
  {
    auto allocInfo = get_resource_allocation_info(device, desc);
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
    {
      allocationFlags = AllocationFlag::NEW_HEAPS_ONLY_WITH_BUDGET;
    }
#endif

    auto memory = allocate(adapter, device, memoryProperties, allocInfo, allocationFlags);

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

      auto errorCode = DX12_CHECK_RESULT_NO_OOM_CHECK(
        device->CreateCommittedResource(&heapProperties, heapFlags, &desc, result.state, nullptr, COM_ARGS(&texture)));

      if (is_oom_error_code(errorCode))
      {
        reportOOMInformation();
      }

      if (!texture)
      {
        logwarn("DX12: Unable to allocate committed resource");
        return result;
      }

      const ImageGlobalSubresouceId subResIdBase = (ii.allocateSubresourceIDs)
                                                     ? allocateGlobalResourceIdRange(ii.getSubResourceCount())
                                                     : ImageGlobalSubresouceId::make_invalid();

      result.image = newImageObject(ResourceMemory{}, eastl::move(texture), ii.type, ii.memoryLayout, ii.format, ii.size, ii.mips,
        ii.arrays, subResIdBase, ii.sampleDesc.Count > 1);

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
      auto errorCode =
#if _TARGET_XBOX
        DX12_CHECK_RESULT_NO_OOM_CHECK(xbox_create_placed_resource(device, memory.getAddress(), desc, result.state, nullptr, texture));
#else
        DX12_CHECK_RESULT_NO_OOM_CHECK(
          device->CreatePlacedResource(memory.getHeap(), memory.getOffset(), &desc, result.state, nullptr, COM_ARGS(&texture)));
#endif

      if (is_oom_error_code(errorCode))
      {
        reportOOMInformation();
      }

      if (!texture)
      {
        free(memory);
        return result;
      }

      const ImageGlobalSubresouceId subResIdBase = (ii.allocateSubresourceIDs)
                                                     ? allocateGlobalResourceIdRange(ii.getSubResourceCount())
                                                     : ImageGlobalSubresouceId::make_invalid();

      result.image = newImageObject(memory, eastl::move(texture), ii.type, ii.memoryLayout, ii.format, ii.size, ii.mips, ii.arrays,
        subResIdBase, ii.sampleDesc.Count > 1);

      updateMemoryRangeUse(memory, result.image);
      recordTextureAllocated(result.image->getMipLevelRange(), result.image->getArrayLayers(), result.image->getBaseExtent(),
        result.image->getMemory().size(), result.image->getFormat(), name);
    }
  }
  else
  {
    ComPtr<ID3D12Resource> texture;
    auto errorCode = DX12_CHECK_RESULT_NO_OOM_CHECK(device->CreateReservedResource(&desc, result.state, nullptr, COM_ARGS(&texture)));

    if (is_oom_error_code(errorCode))
    {
      reportOOMInformation();
    }

    if (!texture)
    {
      return result;
    }

    const ImageGlobalSubresouceId subResIdBase =
      (ii.allocateSubresourceIDs) ? allocateGlobalResourceIdRange(ii.getSubResourceCount()) : ImageGlobalSubresouceId::make_invalid();

    result.image = newImageObject(ResourceMemory{}, eastl::move(texture), ii.type, ii.memoryLayout, ii.format, ii.size, ii.mips,
      ii.arrays, subResIdBase, ii.sampleDesc.Count > 1);
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
    ArrayLayerCount::make(desc.DepthOrArraySize), subResIdBase, false);

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

void TextureImageFactory::destroyTextures(eastl::span<Image *> textures)
{
  for (auto texture : textures)
  {
    if (texture->getGlobalSubResourceIdBase().isValid())
    {
      freeGlobalResourceIdRange(texture->getGlobalSubresourceIdRange());
    }
    if (texture->isHeapAllocated() && !texture->isAliased())
    {
      free(texture->getMemory());
    }
    for (auto &view : texture->getOldViews())
    {
      freeView(view);
    }
    freeView(texture->getRecentView());
    texture->getDebugName([this, texture](auto &name) {
      recordTextureFreed(texture->getMipLevelRange(), texture->getArrayLayers(), texture->getBaseExtent(),
        !texture->isAliased() ? texture->getMemory().size() : 0, texture->getFormat(), name);
    });
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
  if (SBCF_BIND_UNORDERED & desc.cFlags)
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
    if (desc.asBasicRes.cFlags & SBCF_MISC_ESRAM_ONLY)
    {
      return DeviceMemoryClass::ESRAM_RESOURCE;
    }
#endif
    return GenericBufferInterface::get_memory_class(desc.asBasicRes.cFlags);
  }
  else
  {
#if DX12_USE_ESRAM
    if (desc.asBasicRes.cFlags & TEXCF_ESRAM_ONLY)
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

::ResourceHeap *AliasHeapProvider::newUserHeap(DXGIAdapter *adapter, ID3D12Device *device, ::ResourceHeapGroup *group, size_t size,
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

  newPHeap.memory = allocate(adapter, device, properties, allocInfo, allocFlags);
  if (!newPHeap.memory)
  {
    return nullptr;
  }
#ifdef _TARGET_XBOX
  HRESULT hr = device->RegisterPagePoolX(reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS>(newPHeap.memory.asPointer()),
    newPHeap.memory.size() / TEXTURE_TILE_SIZE, &newPHeap.heapRegHandle);
  G_UNUSED(hr);
  G_ASSERT(SUCCEEDED(hr));
#endif
  recordNewUserResourceHeap(size, !properties.isCPUVisible());
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
    logerr("DX12: Tried to free non existing resource heap %p", ptr);
    return;
  }
  auto &heap = (*aliasHeapsAccess)[index];
  if (!heap)
  {
    logerr("DX12: Tried to free already freed resource heap %p", ptr);
    return;
  }
  ResourceHeapProperties properties;
  properties.raw = heap.memory.getHeapID().group;
  recordDeletedUserResouceHeap(heap.memory.size(), !properties.isCPUVisible());

  G_ASSERTF(heap.images.empty() && heap.buffers.empty(), "DX12: Resources of a resource heap should be destroyed before the heap is "
                                                         "destroyed");
#ifdef _TARGET_XBOX
  device->UnregisterPagePoolX(heap.heapRegHandle);
#endif
  G_UNUSED(device);

  free(heap.memory);
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

ImageCreateResult AliasHeapProvider::placeTextureInHeap(ID3D12Device *device, ::ResourceHeap *heap, const ResourceDescription &desc,
  size_t offset, const ResourceAllocationProperties &alloc_info, const char *name)
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
  auto errorCode =
#if _TARGET_XBOX
    DX12_CHECK_RESULT_NO_OOM_CHECK(
      xbox_create_placed_resource(device, memory.getAddress(), dxDesc, result.state, clearValuePtr, texture));
#else
    DX12_CHECK_RESULT_NO_OOM_CHECK(
      device->CreatePlacedResource(memory.getHeap(), memory.getOffset(), &dxDesc, result.state, clearValuePtr, COM_ARGS(&texture)));
#endif

  if (is_oom_error_code(errorCode))
  {
    reportOOMInformation();
  }

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
    subResIdBase, dxDesc.SampleDesc.Count > 1);

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
  return result;
}
#else
ResourceHeapGroupProperties AliasHeapProvider::getResourceHeapGroupProperties(::ResourceHeapGroup *heap_group)
{
  auto properties = getHeapGroupProperties(heap_group);

  ResourceHeapGroupProperties result;
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
    free(heap.memory);
    heap.reset();
  }
}

BufferState AliasHeapProvider::placeBufferInHeap(ID3D12Device *device, ::ResourceHeap *heap, const ResourceDescription &desc,
  size_t offset, const ResourceAllocationProperties &alloc_info, const char *name)
{
  BufferState result;
  // heap ptr / index starts with 1, so adjust to start from 0
  auto index = reinterpret_cast<uintptr_t>(heap) - 1;

  auto aliasHeapsAccess = aliasHeaps.access();
  if (index >= aliasHeapsAccess->size())
  {
    return result;
  }
  auto &heapRef = (*aliasHeapsAccess)[index];

  D3D12_RESOURCE_DESC dxDesc = as_desc(desc);

  D3D12_RESOURCE_STATES state = (SBCF_BIND_UNORDERED & desc.asBufferRes.cFlags) ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                                                                                : D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE;

  BufferHeap::Heap newHeap;
  auto heapProperties = getHeapGroupProperties(alloc_info.heapGroup);
  auto memory = heapRef.memory.aliasSubRange(index, offset, alloc_info.sizeInBytes);

  auto errorCode = newHeap.create(device, dxDesc, memory, state, heapProperties.isCPUVisible());
  if (is_oom_error_code(errorCode))
  {
    reportOOMInformation();
  }

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

ImageCreateResult AliasHeapProvider::aliasTexture(ID3D12Device *device, const ImageInfo &ii, Image *base, const char *name)
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
    auto errorCode =
#if _TARGET_XBOX
      DX12_CHECK_RESULT_NO_OOM_CHECK(xbox_create_placed_resource(device, memory.getAddress(), desc, result.state, nullptr, texture));
#else
      DX12_CHECK_RESULT_NO_OOM_CHECK(
        device->CreatePlacedResource(memory.getHeap(), memory.getOffset(), &desc, result.state, nullptr, COM_ARGS(&texture)));
#endif

    if (is_oom_error_code(errorCode))
    {
      reportOOMInformation();
    }

    if (!texture)
    {
      return result;
    }

    auto subResIdBase = allocateGlobalResourceIdRange(ii.getSubResourceCount());
    result.image = newImageObject(memory, eastl::move(texture), ii.type, ii.memoryLayout, ii.format, ii.size, ii.mips, ii.arrays,
      subResIdBase, ii.sampleDesc.Count > 1);

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
    "Raytrace scratch buffer allocate",
    "Raytrace scratch buffer free",
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
  if (ImGui::BeginChild(id, ImVec2(0, height), true, ImGuiWindowFlags_MenuBar))
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
  ImGui::Selectable(text, false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);
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

using PlotGetFunctionType = ImPlotPoint (*)(void *, int);

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

  static ImPlotPoint getAllocated(void *data, int idx) { return reinterpret_cast<PlotDataBase<T> *>(data)->getAllocatedPoint(idx); }
  static ImPlotPoint getUsed(void *data, int idx) { return reinterpret_cast<PlotDataBase<T> *>(data)->getUsedPoint(idx); }
  static ImPlotPoint getBase(void *data, int idx)
  {
    return {double(reinterpret_cast<PlotDataBase<T> *>(data)->getFrame(idx).frameIndex), double(0)};
  }
};

template <typename T>
void draw_ring_buffer_plot(const char *plot_segment_id, const char *plot_segment_label, const char *plot_label, T select_plot_data)
{
  const int child_height = 17 * ImGui::GetFrameHeightWithSpacing();
  if (begin_sub_section(plot_segment_id, plot_segment_label, child_height))
  {
    auto data = select_plot_data();

    if (ImPlot::BeginPlot(plot_label, nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMousePos | ImPlotFlags_NoLegend, ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit,
          ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
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
    debug("DX12: ResourceMemoryHeap::storeViewSettings 'imgui_get_blk' returned null");
    return;
  }

  auto dx12Block = blk->addBlock("dx12");
  if (!dx12Block)
  {
    debug("DX12: ResourceMemoryHeap::storeViewSettings unable to write dx12 block");
    return;
  }

  auto memoryBlock = dx12Block->addBlock("memory");
  if (!memoryBlock)
  {
    debug("DX12: ResourceMemoryHeap::storeViewSettings unable to write memory block");
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
    debug("DX12: ResourceMemoryHeap::restoreViewSettings 'imgui_get_blk' returned null");
    return;
  }

  auto dx12Block = blk->getBlockByNameEx("dx12");
  if (!dx12Block)
  {
    debug("DX12: ResourceMemoryHeap::restoreViewSettings unable to read dx12 block");
    return;
  }

  auto memoryBlock = dx12Block->getBlockByNameEx("memory");
  if (!memoryBlock)
  {
    debug("DX12: ResourceMemoryHeap::restoreViewSettings unable to read memory block");
    return;
  }
  auto filter = memoryBlock->getStr("event_object_name_filter", "");
  if (filter)
  {
    if (strlen(filter) >= getEventObjectNameFilterMaxLength() - 1)
    {
      logerr("DX12: Restore of dx12/memory/event-object-name-filter with malicious string");
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
    case MetricsState::ActionInfo::Type::RAYTRACE_SCRATCH_BUFFER_ALLOCATE:
    case MetricsState::ActionInfo::Type::RAYTRACE_SCRATCH_BUFFER_FREE:
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
    ImPlot::SetNextPlotLimitsY(0, max_value, ImGuiCond_Always, ImPlotYAxis_2);
    ImPlot::SetNextPlotTicksY(yAxisPointValues, yAxisPointCount, yAxisPintStringPointers, false, ImPlotYAxis_2);
  }
  else
  {
    const char *axisNames[2] = {"", ""};
    ImPlot::SetNextPlotLimits(0, 1, 0, 1, ImGuiCond_Always);
    ImPlot::SetNextPlotTicksX(0, 1, 2, axisNames);
    ImPlot::SetNextPlotTicksY(0, 1, 2, axisNames, false, ImPlotYAxis_2);
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
    ImPlot::SetNextPlotLimitsY(0, max_value, ImGuiCond_Always);
    ImPlot::SetNextPlotTicksY(yAxisPointValues, yAxisPointCount, yAxisPintStringPointers);
  }
  else
  {
    const char *axisNames[2] = {"", ""};
    ImPlot::SetNextPlotLimits(0, 1, 0, 1, ImGuiCond_Always);
    ImPlot::SetNextPlotTicksX(0, 1, 2, axisNames);
    ImPlot::SetNextPlotTicksY(0, 1, 2, axisNames);
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
      ImPlot::SetNextPlotLimitsX(first.frameIndex, first.frameIndex + display_state.windowSize, ImGuiCond_Always);
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

    auto plotData = setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::SYSTEM), maxValue > 0);
    setupPlotYMemoryRange(maxValue, plotData.hasAnyData());

    if (ImPlot::BeginPlot("Memory usage over time##DX12-Memory-Usage-Graph", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMousePos | ImPlotFlags_NoLegend, ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit,
          ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
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

    auto plotData = setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::HEAPS), maxY2 > 0);
    // heap size will always be the largest
    setupPlotYMemoryRange(max(frame.netCountersPeak.gpuHeaps.size, frame.netCountersPeak.cpuHeaps.size), plotData.hasAnyData());
    setupPlotY2CountRange(maxY2, plotData.hasAnyData());

    if (ImPlot::BeginPlot("Usage##DX12-Heap-Graph", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMousePos | ImPlotFlags_NoLegend | ImPlotFlags_YAxis2,
          ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
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

        ImPlot::SetPlotYAxis(ImPlotYAxis_2);

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

    auto plotData = setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::SCRATCH_BUFFER), maxY2 > 0);
    setupPlotYMemoryRange(frame.netCountersPeak.scratchBuffer.size, plotData.hasAnyData());
    setupPlotY2CountRange(maxY2, plotData.hasAnyData());

    if (ImPlot::BeginPlot("Usage##DX12-Scratch-Buffer-Plot", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMousePos | ImPlotFlags_NoLegend | ImPlotFlags_YAxis2,
          ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
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

        ImPlot::SetPlotYAxis(ImPlotYAxis_2);

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

    auto plotData = setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::TEXTURES), frame.netCountersPeak.textures.count);
    // heap size will always be the largest
    setupPlotYMemoryRange(frame.netCountersPeak.textures.size, plotData.hasAnyData());
    setupPlotY2CountRange(frame.netCountersPeak.textures.count, plotData.hasAnyData());

    if (ImPlot::BeginPlot("Usage##DX12-Texture-Graph", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMousePos | ImPlotFlags_NoLegend | ImPlotFlags_YAxis2,
          ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
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

        ImPlot::SetPlotYAxis(ImPlotYAxis_2);

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
    auto plotData = setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::BUFFERS), hasData);

    setupPlotYMemoryRange(max(frame.netCountersPeak.cpuBufferHeaps.size, frame.netCountersPeak.gpuBufferHeaps.size),
      plotData.hasAnyData());
    setupPlotY2CountRange(max(frame.netCountersPeak.cpuBufferHeaps.count, frame.netCountersPeak.gpuBufferHeaps.count),
      plotData.hasAnyData());

    if (ImPlot::BeginPlot("Usage##DX12-Buffer-Heap-Usage-Plot", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMousePos | ImPlotFlags_NoLegend | ImPlotFlags_YAxis2,
          ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
      if (plotData.hasAnyData())
      {
        {
          ScopedPlotStyleVar shadedAlpha{ImPlotStyleVar_FillAlpha, 0.25f};
          plot_shaded("CPU Memory", getPlotPointFrameValue<ImPlotPoint, GetCPUMemory>, getPlotPointFrameBase<ImPlotPoint>, plotData);
          plot_shaded("GPU Memory", getPlotPointFrameValue<ImPlotPoint, GetGPUMemory>, getPlotPointFrameBase<ImPlotPoint>, plotData);
        }
        plot_line("CPU Memory", getPlotPointFrameValue<ImPlotPoint, GetCPUMemory>, plotData);
        plot_line("GPU Memory", getPlotPointFrameValue<ImPlotPoint, GetGPUMemory>, plotData);

        ImPlot::SetPlotYAxis(ImPlotYAxis_2);
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

  auto selectPlotData = [this, peakValue, &metricsAccess]() //
  {
    PlotDataBase<GetPlotData> result //
      {setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::CONST_RING), peakValue > 0)};
    setupPlotYMemoryRange(peakValue, result.hasAnyData());
    return result;
  };

  draw_ring_buffer_plot("DX12-Const-Ring-Usage-Plot-Segment", "Usage", "Usage##DX12-Const-Ring-Usage-Plot", selectPlotData);
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

  auto selectPlotData = [this, peakValue, &metricsAccess]() //
  {
    PlotDataBase<GetPlotData> result //
      {setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::UPLOAD_RING), peakValue > 0)};
    setupPlotYMemoryRange(peakValue, result.hasAnyData());
    return result;
  };

  draw_ring_buffer_plot("DX12-Upload-Ring-Usage-Plot-Segment", "Usage", "Usage##DX12-Upload-Ring-Usage-Plot", selectPlotData);
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

  auto selectPlotData = [this, peakValue, &metricsAccess]() //
  {
    PlotDataBase<GetPlotData> result //
      {setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::TEMP_MEMORY), peakValue > 0)};
    setupPlotYMemoryRange(peakValue, result.hasAnyData());
    return result;
  };

  draw_ring_buffer_plot("DX12-Temp-Buffer-Usage-Plot-Segment", "Usage", "Usage##DX12-Temp-Buffer-Usage-Plot", selectPlotData);
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
    maxY2 = max(maxY2, frame.netCountersPeak.raytraceScratchBuffer.count);

    auto plotData = setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::RAYTRACING), maxY2 > 0);
    uint64_t maxY = frame.netCountersPeak.raytraceTopStructure.size;
    maxY = max(maxY, frame.netCountersPeak.raytraceBottomStructure.size);
    maxY = max(maxY, frame.netCountersPeak.raytraceScratchBuffer.size);

    setupPlotYMemoryRange(maxY, plotData.hasAnyData());
    setupPlotY2CountRange(maxY2, plotData.hasAnyData());

    if (ImPlot::BeginPlot("Usage##DX12-raytracing-plot", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMousePos | ImPlotFlags_NoLegend | ImPlotFlags_YAxis2,
          ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
      if (plotData.hasAnyData())
      {
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
        struct GetScratchSize
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.raytraceScratchBuffer.size); }
        };
        struct GetScratchCount
        {
          static double get(const PerFrameData &frame) { return double(frame.netCounters.raytraceScratchBuffer.count); }
        };
        {
          ScopedPlotStyleVar shadedAlpha{ImPlotStyleVar_FillAlpha, 0.25f};
          plot_shaded("Top Structure Memory", getPlotPointFrameValue<ImPlotPoint, GetTopSize>, getPlotPointFrameBase<ImPlotPoint>,
            plotData);

          plot_shaded("Bottom Structure Memory", getPlotPointFrameValue<ImPlotPoint, GetBottomSize>,
            getPlotPointFrameBase<ImPlotPoint>, plotData);

          plot_shaded("Scratch Memory", getPlotPointFrameValue<ImPlotPoint, GetScratchSize>, getPlotPointFrameBase<ImPlotPoint>,
            plotData);
        }

        plot_line("Top Structure Memory", getPlotPointFrameValue<ImPlotPoint, GetTopSize>, plotData);
        plot_line("Bottom Structure Memory", getPlotPointFrameValue<ImPlotPoint, GetBottomSize>, plotData);
        plot_line("Scratch Memory", getPlotPointFrameValue<ImPlotPoint, GetScratchSize>, plotData);

        ImPlot::SetPlotYAxis(ImPlotYAxis_2);

        plot_line("Top Structure Count", getPlotPointFrameValue<ImPlotPoint, GetTopCount>, plotData);
        plot_line("Bottom Structure Count", getPlotPointFrameValue<ImPlotPoint, GetBottomCount>, plotData);
        plot_line("Scratch Count", getPlotPointFrameValue<ImPlotPoint, GetScratchCount>, plotData);

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

              ByteUnits scratchSize = frame.netCounters.raytraceScratchBuffer.size;
              ImPlot::ItemIcon(ImPlot::GetColormapColor(2));
              ImGui::SameLine();
              ImGui::Text("Scratch Memory: %.2f %s", scratchSize.units(), scratchSize.name());

              ImPlot::ItemIcon(ImPlot::GetColormapColor(3));
              ImGui::SameLine();
              ImGui::Text("Top Structure Count: %I64u", frame.netCounters.raytraceTopStructure.count);

              ImPlot::ItemIcon(ImPlot::GetColormapColor(4));
              ImGui::SameLine();
              ImGui::Text("Bottom Structure Count: %I64u", frame.netCounters.raytraceBottomStructure.count);

              ImPlot::ItemIcon(ImPlot::GetColormapColor(5));
              ImGui::SameLine();
              ImGui::Text("Scratch Count: %I64u", frame.netCounters.raytraceScratchBuffer.count);
            }
          }
        }
      }
      else
      {
        // When no data is available just display a empty plot
        ImPlot::PlotDummy("Top Structure Memory");
        ImPlot::PlotDummy("Bottom Structure Memory");
        ImPlot::PlotDummy("Scratch Memory");
        ImPlot::PlotDummy("Top Structure Count");
        ImPlot::PlotDummy("Bottom Structure Count");
        ImPlot::PlotDummy("Scratch Count");
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

    auto plotData = setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::USER_HEAPS), maxY2 > 0);
    // heap size will always be the largest
    setupPlotYMemoryRange(frame.netCountersPeak.userResourceHeaps.size, plotData.hasAnyData());
    setupPlotY2CountRange(maxY2, plotData.hasAnyData());

    if (ImPlot::BeginPlot("Usage##DX12-User-Heap-Graph", nullptr, nullptr, ImVec2(-1, 0),
          ImPlotFlags_NoTitle | ImPlotFlags_NoMousePos | ImPlotFlags_NoLegend | ImPlotFlags_YAxis2,
          ImPlotAxisFlags_Invert | ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_LockMin | ImPlotAxisFlags_AutoFit))
    {
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

        ImPlot::SetPlotYAxis(ImPlotYAxis_2);

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

  auto selectPlotData = [this, peakValue, &metricsAccess]() //
  {
    PlotDataBase<GetPlotData> result //
      {setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::UPLOAD_MEMORY), peakValue > 0)};
    setupPlotYMemoryRange(peakValue, result.hasAnyData());
    return result;
  };

  draw_ring_buffer_plot("DX12Persistent-Upload-Memory-Usage-Plot-Segment", "Usage", "Usage##DX12Persistent-Upload-Memory-Usage-Plot",
    selectPlotData);
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

  auto selectPlotData = [this, peakValue, &metricsAccess]() //
  {
    PlotDataBase<GetPlotData> result //
      {setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::READ_BACK_MEMORY), peakValue > 0)};
    setupPlotYMemoryRange(peakValue, result.hasAnyData());
    return result;
  };

  draw_ring_buffer_plot("DX12Persistent-Read-Back-Memory-Usage-Plot-Segment", "Usage",
    "Usage##DX12Persistent-Read-Back-Memory-Usage-Plot", selectPlotData);
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

  auto selectPlotData = [this, peakValue, &metricsAccess]() //
  {
    PlotDataBase<GetPlotData> result //
      {setupPlotXRange(metricsAccess, drawGraphViewControls(Graph::BIDIRECTIONAL_MEMORY), peakValue > 0)};
    setupPlotYMemoryRange(peakValue, result.hasAnyData());
    return result;
  };

  draw_ring_buffer_plot("DX12Persistent-Bidirectional-Memory-Usage-Plot-Segment", "Usage",
    "Usage##DX12Persistent-Bidirectional-Memory-Usage-Plot", selectPlotData);
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

  begin_selectable_row("Scratch buffer size");
  ByteUnits scratchSize = frameCounters.netCounters.raytraceScratchBuffer.size;
  ImGui::TableNextColumn();
  ImGui::Text("%.2f %s", scratchSize.units(), scratchSize.name());
  scratchSize = frameCounters.netCountersPeak.raytraceScratchBuffer.size;
  ImGui::TableNextColumn();
  ImGui::Text("%.2f %s", scratchSize.units(), scratchSize.name());

  begin_selectable_row("Scratch buffer count");
  ImGui::TableNextColumn();
  ImGui::Text("%I64u", frameCounters.netCounters.raytraceScratchBuffer.count);
  ImGui::TableNextColumn();
  ImGui::Text("%I64u", frameCounters.netCountersPeak.raytraceScratchBuffer.count);

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

    raytraceAccelerationStructurePool.access()->iterateAllocated([](auto as) //
      {
        // currently the only indicator for top or bottom is to see if it has a UAV descriptor or
        // not if not then its bottom
        if (0 == as->handle.ptr)
        {
          return;
        }

        char buf[64];
        sprintf_s(buf, "%016I64X", as->getGPUPointer());
        begin_selectable_row(buf);

        ImGui::TableNextColumn();
        ImGui::Text("%016I64X", as->getGPUPointer() + as->size());

        ByteUnits sz = as->size();
        ImGui::TableNextColumn();
        ImGui::Text("%.2f %s", sz.units(), sz.name());
      });

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

    raytraceAccelerationStructurePool.access()->iterateAllocated([](auto as) //
      {
        // currently the only indicator for top or bottom is to see if it has a UAV descriptor or
        // not if not then its bottom
        if (0 != as->handle.ptr)
        {
          return;
        }

        char buf[64];
        sprintf_s(buf, "%016I64X", as->getGPUPointer());
        begin_selectable_row(buf);

        ImGui::TableNextColumn();
        ImGui::Text("%016I64X", as->getGPUPointer() + as->size());

        ByteUnits sz = as->size();
        ImGui::TableNextColumn();
        ImGui::Text("%.2f %s", sz.units(), sz.name());
      });

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
          ByteUnits freeSize = eastl::accumulate(begin(heap.freeRanges), end(heap.freeRanges), 0,
            [](size_t size, ValueRange<uint32_t> range) { return size + range.size(); });
          ByteUnits readySize = eastl::accumulate(begin(bufferHeapStateAccess->bufferHeapDiscardStandbyList),
            end(bufferHeapStateAccess->bufferHeapDiscardStandbyList), 0,
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
        end(bufferHeapStateAccess->bufferHeapDiscardStandbyList), 0,
        [&bufferHeapStateAccess](size_t size, auto &info) //
        { return size + ((nullptr == bufferHeapStateAccess->bufferHeaps[info.index].getCPUPointer()) ? info.range.size() : 0); });
      systemStandbyAllocateSize = eastl::accumulate(begin(bufferHeapStateAccess->bufferHeapDiscardStandbyList),
        end(bufferHeapStateAccess->bufferHeapDiscardStandbyList), 0,
        [&bufferHeapStateAccess](size_t size, auto &info) //
        { return size + ((nullptr != bufferHeapStateAccess->bufferHeaps[info.index].getCPUPointer()) ? info.range.size() : 0); });

      for (const auto &heap : bufferHeapStateAccess->bufferHeaps)
      {
        if (heap)
        {
          auto freeMem = eastl::accumulate(begin(heap.freeRanges), end(heap.freeRanges), 0,
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
      for (properties.raw = 0; properties.raw < countof(groups); ++properties.raw)
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
            ImGui::Text("%zu", heap->usedRanges.size());

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
                  else if (eastl::holds_alternative<RaytraceScratchBufferReference>(res))
                  {
                    draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Raytracing scratch buffer");
                  }
                  else if (eastl::holds_alternative<RaytraceBottomLevelAccelerationStructureRefnerence>(res))
                  {
                    draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Raytracing bottom level acceleration structure");
                  }
                  else if (eastl::holds_alternative<RaytraceTopLevelAccelerationStructureRefnerence>(res))
                  {
                    draw_segment(heap->heapPointer(), *segment, heap->totalSize, "Raytracing top level acceleration structure");
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

struct ResourceHeapVisitor
{
  String &str;
  ByteUnits &totalSize;
  ByteUnits &freeSize;
  size_t &totalHeapCount;

  void visitHeapGroup(uint32_t ident, size_t count, bool is_cpu_visible, bool is_cpu_cached, bool is_gpu_executable)
  {
    str.aprintf(128, "Heap Group %08X (%s, %s%s) with %d heaps\n", ident, is_cpu_visible ? "CPU visible" : "dedicated GPU",
      is_cpu_visible ? is_cpu_cached ? "CPU cached, " : "CPU write combine, " : "",
      is_gpu_executable ? "GPU executable" : "Not GPU executable", count);
    totalHeapCount += count;
  }

  void visitHeap(ByteUnits total_size, ByteUnits free_size, uint32_t fragmentation_percent)
  {
    totalSize += total_size;
    freeSize += free_size;
    str.aprintf(128, "Size %6.2f %7s Free %6.2f %7s, %3u%% fragmentation\n", total_size.units(), total_size.name(), free_size.units(),
      free_size.name(), fragmentation_percent);
  }
};
} // namespace

void ResourceMemoryHeap::generateResourceAndMemoryReport(uint32_t *num_textures, uint64_t *total_mem, String *out_text)
{
  if (num_textures)
    *num_textures = 0;
  if (total_mem)
    *total_mem = 0;

  if (out_text)
  {
    out_text->append("\n");
    out_text->append("~ Resource Heaps ~~~~~~~~~~~~~~~~~~~~\n");
    ByteUnits totalResourceMemorySize;
    ByteUnits freeResourceMemorySize;
    size_t totalHeapCount = 0;
    visitHeaps(ResourceHeapVisitor{*out_text, totalResourceMemorySize, freeResourceMemorySize, totalHeapCount});
    out_text->aprintf(128, "%u resource heaps, with %6.2f %7s in total and %6.2f %7s free\n", totalHeapCount,
      totalResourceMemorySize.units(), totalResourceMemorySize.name(), freeResourceMemorySize.units(), freeResourceMemorySize.name());

    ByteUnits pushRingSize = getFramePushRingMemorySize();
    ByteUnits uploadRingSize = getUploadRingMemorySize();
    ByteUnits tempUploadSize = getTemporaryUploadMemorySize();
    ByteUnits persistUploadSize = getPersistentUploadMemorySize();
    ByteUnits persistReadBackSize = getPersistentReadBackMemorySize();
    ByteUnits persistBidirSize = getPersistentBidirectionalMemorySize();

    out_text->append("~ Driver internal buffers ~~~~~~~~~~~\n");
    out_text->aprintf(128, "%6.2f %7s push ring buffers\n", pushRingSize.units(), pushRingSize.name());
    out_text->aprintf(128, "%6.2f %7s upload ring buffers\n", uploadRingSize.units(), uploadRingSize.name());
    out_text->aprintf(128, "%6.2f %7s temporary upload buffers\n", tempUploadSize.units(), tempUploadSize.name());
    out_text->aprintf(128, "%6.2f %7s persistent upload buffers\n", persistUploadSize.units(), persistUploadSize.name());
    out_text->aprintf(128, "%6.2f %7s persistent read back buffers\n", persistReadBackSize.units(), persistReadBackSize.name());
    out_text->aprintf(128, "%6.2f %7s persistent bidirectional buffers\n", persistBidirSize.units(), persistBidirSize.name());
    out_text->append("~ All driver level buffers ~~~~~~~~~~\n");
    ByteUnits butterTotalExternalSize;
    ByteUnits bufferTotalInternalSize;
    size_t bufferCount = 0;
    size_t discardReadyCount = 0;
    visitBuffers([out_text, &butterTotalExternalSize, &bufferTotalInternalSize, &bufferCount, &discardReadyCount](const char *name,
                   ByteUnits internal_size, ByteUnits external_size,
                   bool is_discard_ready) //
      {
        ++bufferCount;
        if (is_discard_ready)
        {
          ++discardReadyCount;
        }
        butterTotalExternalSize += external_size;
        bufferTotalInternalSize += internal_size;
        out_text->aprintf(128, "%6.2f %7s - %6.2f %7s %s%s\n", internal_size.units(), internal_size.name(), external_size.units(),
          external_size.name(), name, is_discard_ready ? " discard ready" : "");
      });
    out_text->aprintf(128,
      "%u buffers of which %u are discard ready list, with %6.2f %7s external size "
      "and %6.2f %7s internal size\n",
      bufferCount, discardReadyCount, butterTotalExternalSize.units(), butterTotalExternalSize.name(), bufferTotalInternalSize.units(),
      bufferTotalInternalSize.name());
    out_text->append("~ Sbuffer resources ~~~~~~~~~~~~~~~~~\n");

    {
      ByteUnits bufferTotalSize = 0;
      ByteUnits bufferSysCopySize = 0;
      size_t sbufferCount = 0;
      visitBufferObjects([out_text, &bufferTotalSize, &bufferSysCopySize, &sbufferCount](auto buffer) //
        {
          ++sbufferCount;
          ByteUnits size = buffer->ressize();
          bufferTotalSize += size;
          if (buffer->hasSystemCopy())
          {
            bufferSysCopySize += size;
          }
          out_text->aprintf(128, "%6.2f %7s %s '%s' buffer%s\n", size.units(), size.name(),
            buffer->getFlags() & SBCF_DYNAMIC ? "dynamic" : "static", buffer->getBufName(),
            buffer->hasSystemCopy() ? "(has copy)" : "");
        });
      out_text->aprintf(256, "%u buffers, with %6.2f %7s buffer memory and %6.2f %7s system memory\n", sbufferCount,
        bufferTotalSize.units(), bufferTotalSize.name(), bufferSysCopySize.units(), bufferSysCopySize.name());
    }

    out_text->append("~ All driver level textures ~~~~~~~~~\n");
    ByteUnits totalImageResourcesSize;
    size_t imageCount = 0;
    size_t imageAliases = 0;
    visitImageObjects([out_text, &totalImageResourcesSize, &imageCount, &imageAliases](auto image) //
      {
        if (image->isAliased())
        {
          ++imageAliases;
          image->getDebugName([&out_text](auto &name) { out_text->aprintf(128, "%14s '%s'\n", "aliased", name); });
        }
        else
        {
          ByteUnits size = image->getMemory().size();
          totalImageResourcesSize += size;
          image->getDebugName(
            [&out_text, size](auto &name) { out_text->aprintf(128, "%6.2f %7s '%s'\n", size.units(), size.name(), name); });
        }
        ++imageCount;
      });
    out_text->aprintf(128, "%u textures, with %6.2f %7s plus %u aliased textures\n", imageCount, totalImageResourcesSize.units(),
      totalImageResourcesSize.name(), imageAliases);
  }

  uint32_t tex2DCount = 0;
  uint32_t texCubeCount = 0;
  uint32_t texVolCount = 0;
  uint32_t texArrayCount = 0;

  ByteUnits tex2DSize = 0;
  ByteUnits texCubeSize = 0;
  ByteUnits texVolSize = 0;
  ByteUnits texArraySize = 0;

  if (out_text)
  {
    out_text->append("~ Texture resources ~~~~~~~~~~~~~~~~~\n");
  }
  {
    String tmpStor;
    visitTextureObjects([&](auto tex) //
      {
        if (auto img = tex->getDeviceImage())
        {
          ByteUnits imageBytes = img->getMemory().size();
          if (img->isAliased())
          {
            imageBytes = 0;
          }
          auto ql = tql::get_tex_cur_ql(tex->getTID());
          switch (tex->resType)
          {
            case RES3D_TEX:
              ++tex2DCount;
              tex2DSize += imageBytes.value();
              if (out_text)
              {
                out_text->aprintf(0,
                  "%6.2f %7s q%d  2d  %4dx%-4d, m%2d, aniso=%-2d filter=%7s mip=%s, %s "
                  "- '%s'",
                  imageBytes.units(), imageBytes.name(), ql, tex->width, tex->height, tex->level_count(), tex->samplerState.getAniso(),
                  filter_type_to_string(tex->samplerState.getFilter()), filter_type_to_string(tex->samplerState.getMip()),
                  img->getFormat().getNameString(), tex->getResName());
              }
              break;
            case RES3D_CUBETEX:
              ++texCubeCount;
              texCubeSize += imageBytes.value();
              if (out_text)
              {
                out_text->aprintf(0, "%6.2f %7s q%d cube %4d, m%2d, %s - '%s'", imageBytes.units(), imageBytes.name(), ql, tex->width,
                  tex->level_count(), img->getFormat().getNameString(), tex->getResName());
              }
              break;
            case RES3D_VOLTEX:
              ++texVolCount;
              texVolSize += imageBytes.value();
              if (out_text)
              {
                out_text->aprintf(0,
                  "%6.2f %7s q%d  vol %4dx%-4dx%3d, m%2d, aniso=%-2d filter=%7s mip=%s, "
                  "%s - '%s'",
                  imageBytes.units(), imageBytes.name(), ql, tex->width, tex->height, tex->depth, tex->level_count(),
                  tex->samplerState.getAniso(), filter_type_to_string(tex->samplerState.getFilter()),
                  filter_type_to_string(tex->samplerState.getMip()), img->getFormat().getNameString(), tex->getResName());
              }
              break;
            case RES3D_ARRTEX:
              ++texArrayCount;
              texArraySize += imageBytes.value();
              if (out_text)
              {
                out_text->aprintf(0,
                  "%6.2f %7s q%d  arr %4dx%-4dx%3d, m%2d, aniso=%-2d filter=%7s mip=%s, "
                  "%s - '%s'",
                  imageBytes.units(), imageBytes.name(), ql, tex->width, tex->height, tex->depth, tex->level_count(),
                  tex->samplerState.getAniso(), filter_type_to_string(tex->samplerState.getFilter()),
                  filter_type_to_string(tex->samplerState.getMip()), img->getFormat().getNameString(), tex->getResName());
              }
              break;
          }
          if (out_text)
          {
            if (img->isAliased())
            {
              out_text->append(" is alias");
            }
            if (num_textures)
            {
              out_text->append("\n");
            }
            else
            {
              out_text->aprintf(0, " cache=%d%s\n", img->getViewCount(), tql::get_tex_info(tex->getTID(), false, tmpStor));
            }
          }
        }
      });
  }

  auto totalCount = tex2DCount + texCubeCount + texVolCount + texArrayCount;
  auto totalMem = tex2DSize + texCubeSize + texVolSize + texArraySize;

  if (num_textures)
    *num_textures = totalCount;
  if (total_mem)
    *total_mem = totalMem.value();

  if (out_text)
  {
    out_text->aprintf(256,
      "Total: Texture %6.2f %7s over %u textures, 2D %6.2f %7s over %u, Cube %6.2f %7s "
      "over %u, Vol %6.2f %7s over %u, Array %6.2f %7s over %u\n",
      totalMem.units(), totalMem.name(), totalCount, tex2DSize.units(), tex2DSize.name(), tex2DCount, texCubeSize.units(),
      texCubeSize.name(), texCubeCount, texVolSize.units(), texVolSize.name(), texVolCount, texArraySize.units(), texArraySize.name(),
      texArrayCount);
  }
}

void OutOfMemoryRepoter::reportOOMInformation()
{
  if (!didReportOOM)
  {
    didReportOOM = true;
    String texStat;
    // cast assumes that OutOfMemoryRepoter is a base of ResourceMemoryHeap
    G_STATIC_ASSERT((eastl::is_base_of<OutOfMemoryRepoter, ResourceMemoryHeap>::value));
    // can't use static cast as it respects visibility rules
    ((ResourceMemoryHeap *)this)->generateResourceAndMemoryReport(nullptr, nullptr, &texStat);
    debug("GPU MEMORY STATISTICS:\n======================");
    debug(texStat.str());
    debug("======================\nEND GPU MEMORY STATISTICS");
  }
  else
  {
    debug("GPU MEMORY STATISTICS: not reported as this is not the first time.");
  }
}
