#pragma once

#include <dag/dag_vector.h>
#if _TARGET_XBOX
#if _TARGET_SCARLETT
#include <xg_xs.h>
#else
#include <xg.h>
#endif
#include <EASTL/intrusive_ptr.h>
inline void intrusive_ptr_add_ref(XGTextureAddressComputer *ptr) { ptr->AddRef(); }

inline void intrusive_ptr_release(XGTextureAddressComputer *ptr) { ptr->Release(); }
#endif

#if DX12_USE_ESRAM
#include <xgmemory.h>
namespace drv3d_dx12
{
struct EsramResource
{
  XGMemoryLayoutMapping mapping{};
  Image *dramStorage = nullptr;
  int mappingIndex = -1;

  explicit operator bool() const { return -1 != mappingIndex; }
};
} // namespace drv3d_dx12
#endif

namespace drv3d_dx12
{
// size is not important here
struct ImageInfo
{
  D3D12_RESOURCE_DIMENSION type;
  D3D12_RESOURCE_FLAGS usage;
  Extent3D size;
  ArrayLayerCount arrays;
  MipMapCount mips;
  FormatStore format;
  D3D12_TEXTURE_LAYOUT memoryLayout;
  DeviceMemoryClass memoryClass;
  bool allocateSubresourceIDs;

  SubresourceCount getSubResourceCount() const { return SubresourcePerFormatPlaneCount::make(mips, arrays) * format.getPlanes(); }

  D3D12_RESOURCE_DESC asDesc() const
  {
    D3D12_RESOURCE_DESC desc;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = memoryLayout;
    desc.Flags = usage;
    desc.Format = format.asDxGiTextureCreateFormat();
    desc.Dimension = type;
    desc.Width = size.width;
    desc.MipLevels = mips.count();
    desc.Alignment = calculate_texture_alignment(size.width, size.height, size.depth, 1, memoryLayout, usage, format);
    switch (type)
    {
      default:
      case D3D12_RESOURCE_DIMENSION_UNKNOWN:
      case D3D12_RESOURCE_DIMENSION_BUFFER: fatal("DX12: Invalid texture dimension"); return desc;
      case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        desc.Height = 1;
        desc.DepthOrArraySize = arrays.count();
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        desc.Height = size.height;
        desc.DepthOrArraySize = arrays.count();
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        desc.Height = size.height;
        desc.DepthOrArraySize = size.depth;
        break;
    }
    return desc;
  }
};

#define DX12_IMAGE_DEBUG_NAMES 1
class Image
{
public:
  using ViewInfo = ImageViewInfo;

private:
  ComPtr<ID3D12Resource> image;
  ResourceMemory memory;
  Extent3D extent{};
  typedef dag::Vector<ViewInfo> old_views_t;

  ViewInfo recentView = {}; // most recent view
  old_views_t oldViews;
#if DX12_USE_ESRAM
  EsramResource esramResource{};
#endif
  ContainerMutexWrapper<String, OSSpinlock> debugName;

  D3D12_RESOURCE_DIMENSION imageType = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  ArrayLayerCount layerCount{};
  MipMapCount mipLevels{};
  FormatStore format{};
  ExtendedImageGlobalSubresouceId globalSubResBase;
#if _TARGET_XBOX
  eastl::intrusive_ptr<XGTextureAddressComputer> textureAccessComputer;
  D3D12_TEXTURE_LAYOUT textureLayout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
#endif
  uint64_t lastFrameAccess = 0;

public:
  void updateLastFrameAccess(uint64_t index) { lastFrameAccess = index; }
  bool wasAccessedPreviousFrames(uint64_t index) const
  {
    // A diff of 0 means it was used during this frame before this upload is executed, this is not
    // without problems.
    // A diff of from 1 to FRAME_FRAME_BACKLOG_LENGTH-1 means it was used during the last frames which could be
    // still inflight on the GPU so synchronization is needed, otherwise we risk overlap.
    // A diff of more than FRAME_FRAME_BACKLOG_LENGTH-1 means it was used during frames that already executed
    // on the GPU, so its fine.
    return (index - lastFrameAccess) < FRAME_FRAME_BACKLOG_LENGTH;
  }
  bool wasAccessedThisFrame(uint64_t index) const { return lastFrameAccess == index; }
#if _TARGET_XBOX
  XGTextureAddressComputer *getAccessComputer()
  {
    if (!textureAccessComputer)
    {
      auto desc = image->GetDesc();
      G_STATIC_ASSERT(sizeof(XG_RESOURCE_DESC) == sizeof(D3D12_RESOURCE_DESC));
      desc.Layout = textureLayout;
      XGTextureAddressComputer *ptr = nullptr;
      // XG_RESOURCE_DESC is a replication of D3D12_RESOURCE_DESC
      XGCreateTextureComputer(reinterpret_cast<XG_RESOURCE_DESC *>(&desc), &ptr);
      textureAccessComputer.attach(ptr);
    }
    return textureAccessComputer.get();
  }
  void releaseAccessComputer() { textureAccessComputer.reset(); }
#endif
  bool isGPUChangeable() const { return !globalSubResBase.isStatic(); }
  void setGPUChangeable(bool b)
  {
    if (b)
    {
      globalSubResBase.setNonStatic();
    }
    else
    {
      globalSubResBase.setStatic();
    }
  }
  void setReportStateTransitions() { globalSubResBase.enableTransitionReporting(); }
  bool shouldReportStateTransitions() const { return globalSubResBase.shouldReportTransitions(); }
  size_t getViewCount() const { return oldViews.size() + (recentView.state.isValid() ? 1 : 0); }
  bool isAliased() const { return 0 != memory.getHeapID().isAlias; }
  bool isReserved() const { return !isHeapAllocated(); }
  ResourceMemory getMemory() const { return memory; }
  bool isHeapAllocated() const { return static_cast<bool>(memory); }
  template <typename T>
  void getDebugName(T clb)
  {
    clb(*debugName.access());
  }
  bool hasMoreThanOneSubresource() const { return (mipLevels.count() > 1) || (layerCount.count() > 1); }
  bool isValidResId(uint32_t res_id) const { return res_id < (mipLevels.count() * layerCount.count()); }
  ID3D12Resource *getHandle() const { return image.Get(); }
  FormatStore getFormat() const { return format; }
  const old_views_t &getOldViews() const { return oldViews; }
  void removeAllViews()
  {
    oldViews.clear();
    recentView = {};
  }
  const ViewInfo &getRecentView() const { return recentView; }
  void updateRecentView(old_views_t::const_iterator iter)
  {
    eastl::swap(oldViews[eastl::distance(oldViews.cbegin(), iter)], recentView); // update LRU entry
  }
  void addView(const ViewInfo &view)
  {
    if (recentView.state.isValid())
      oldViews.push_back(recentView);
    recentView = view;
  }
  D3D12_RESOURCE_DIMENSION getType() const { return imageType; }
  void setDebugName(const char *name) { *debugName.access() = name; }
  ArrayLayerCount getArrayLayers() const { return layerCount; }
  ArrayLayerCount getArrayLayerRange() const { return getArrayLayers(); }
  MipMapCount getMipLevelRange() const { return mipLevels; }
  SubresourcePerFormatPlaneCount getSubresourcesPerPlane() const { return mipLevels * layerCount; }
  FormatPlaneCount getPlaneCount() const { return format.getPlanes(); }
  SubresourceRange getSubresourceRange() const { return getSubresourcesPerPlane() * format.getPlanes(); }
  ValueRange<ExtendedImageGlobalSubresouceId> getGlobalSubresourceIdRange() const
  {
    if (globalSubResBase.isValid())
    {
      return {globalSubResBase, globalSubResBase.add(getSubresourcesPerPlane() * format.getPlanes())};
    }
    else
    {
      return {};
    }
  }

  ExtendedImageGlobalSubresouceId getGlobalSubResourceIdBase() const { return globalSubResBase; }
  bool hasTrackedState() const { return globalSubResBase.isValid(); }

  // Handles count of 0 and validates inputs. On invalid inputs it returns a invalid range.
  SubresourceRange getSubresourceRangeForBarrier(uint32_t offset, uint32_t count)
  {
    // TODO: use correct types for offset / count (eg range)
    // If a format has more than one plane, the driver does replicate the barrier to the other
    // planes, so we have only a range of the first plane
    auto limit = getSubresourcesPerPlane();
    G_ASSERTF_RETURN(offset < limit.count(), SubresourceRange::make_invalid(),
      "DX12: Invalid subresouce index %u in barrier for texture <%s>, texture has "
      "only only %u subresources",
      offset, *debugName.access(), limit);
    if (!count)
    {
      count = limit.count() - offset;
    }
    G_ASSERTF_RETURN(offset + count <= limit.count(), SubresourceRange::make_invalid(),
      "DX12: Invalid subresouce range from %u to %u in barrier for texture <%s>, "
      "texture has only %u subresources",
      offset, offset + count, *debugName.access(), limit);
    return SubresourceRange::make(offset, count);
  }

  MipMapIndex stateIndexToMipIndex(SubresourceIndex id) const
  {
    return MipMapIndex::make(calculate_mip_slice_from_index(id.index(), mipLevels.count()));
  }
  ArrayLayerIndex stateIndexToArrayIndex(SubresourceIndex id) const
  {
    return ArrayLayerIndex::make(calculate_array_slice_from_index(id.index(), mipLevels.count(), layerCount.count()));
  }
  SubresourceIndex mipAndLayerIndexToStateIndex(MipMapIndex mip, ArrayLayerIndex layer) const
  {
    if (imageType == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
      layer = ArrayLayerIndex::make(0);
    }
    return SubresourceIndex::make(calculate_subresource_index(mip.index(), layer.index(), 0, mipLevels.count(), layerCount.count()));
  }
  SubresourceIndex mipAndLayerResourceIndex(MipMapIndex mip, ArrayLayerIndex layer, FormatPlaneIndex plane_index) const
  {
    if (imageType == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
      layer = ArrayLayerIndex::make(0);
    }
    return SubresourceIndex::make(
      calculate_subresource_index(mip.index(), layer.index(), plane_index.index(), mipLevels.count(), layerCount.count()));
  }

  Image(ResourceMemory mem, ComPtr<ID3D12Resource> img, D3D12_RESOURCE_DIMENSION type, D3D12_TEXTURE_LAYOUT layout, FormatStore fmt,
    Extent3D ext, MipMapCount levels, ArrayLayerCount layers, ImageGlobalSubresouceId sub_res_base) :
    memory{mem},
    imageType(type),
#ifdef _TARGET_XBOX
    textureLayout(layout),
#endif
    layerCount(layers),
    image(eastl::move(img)),
    format(fmt),
    extent(ext),
    mipLevels(levels),
    globalSubResBase{ExtendedImageGlobalSubresouceId::make(sub_res_base)}
  {
    G_UNUSED(layout);
  }

#if DX12_USE_ESRAM
  Image(ResourceMemory mem, ComPtr<ID3D12Resource> img, D3D12_RESOURCE_DIMENSION type, FormatStore fmt, Extent3D ext,
    MipMapCount levels, ArrayLayerCount layers, ImageGlobalSubresouceId sub_res_base, const EsramResource &esram_resource) :
    memory{mem},
    imageType(type),
    layerCount(layers),
    image(eastl::move(img)),
    format(fmt),
    extent(ext),
    mipLevels(levels),
    globalSubResBase{ExtendedImageGlobalSubresouceId::make(sub_res_base)},
    esramResource{esram_resource}
  {}

  EsramResource &getEsramResource() { return esramResource; }
  const EsramResource &getEsramResource() const { return esramResource; }
  bool isEsramTexture() const { return esramResource.mappingIndex != -1; }
  void resetEsram() { esramResource = {}; }
#endif

  const Extent3D &getBaseExtent() const { return extent; }

  Extent3D getMipExtents(MipMapIndex level) const { return mip_extent(extent, level.index()); }

  Extent2D getMipExtents2D(MipMapIndex level) const
  {
    Extent2D me = {max(extent.width >> level.index(), 1u), max(extent.height >> level.index(), 1u)};
    return me;
  }

  uint32_t getMipHeight(MipMapIndex level) const { return max(extent.height >> level.index(), 1u); }

  D3D_SRV_DIMENSION getSRVDimension() const
  {
    switch (imageType)
    {
      default: fatal("Unreachable switch case"); return D3D_SRV_DIMENSION_TEXTURE2D;
      case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        return layerCount.count() > 1 ? D3D_SRV_DIMENSION_TEXTURE1DARRAY : D3D_SRV_DIMENSION_TEXTURE1D;
      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        return layerCount.count() > 1 ? D3D_SRV_DIMENSION_TEXTURE2DARRAY : D3D_SRV_DIMENSION_TEXTURE2D;
      case D3D12_RESOURCE_DIMENSION_TEXTURE3D: return D3D_SRV_DIMENSION_TEXTURE3D;
    }
  }

  // Detaches the current resource and attaches the given one.
  // (detach/attach do not interact with refcouting, simple pointer swap)
  void replaceResource(ID3D12Resource *resource)
  {
    image.Detach();
    image.Attach(resource);
  }

  void replaceMemory(ResourceMemory m) { memory = m; }

  void updateExtents(Extent3D new_extent) { extent = new_extent; }

  void updateFormat(FormatStore fmt) { format = fmt; }
};

struct ImageCreateResult
{
  Image *image;
  D3D12_RESOURCE_STATES state;
};
namespace resource_manager
{
class BufferObjectProvider : public GlobalSubresourceIdProvider
{
  using BaseType = GlobalSubresourceIdProvider;

protected:
  OSSpinlock bufferPoolGuard;
  ObjectPool<GenericBufferInterface> bufferPool;

  void shutdown()
  {
    OSSpinlockScopedLock lock{bufferPoolGuard};
    auto sz = bufferPool.size();
    G_ASSERTF(0 == sz, "DX12: Shutdown without destroying all buffers, there are still %u buffers alive!", sz);
#if DAGOR_DBGLEVEL > 0
    bufferPool.iterateAllocated([](auto buffer) { G_ASSERTF(false, "DX12: Buffer <%s> still alive!", buffer->getBufName()); });
#endif
    G_UNUSED(sz);
    bufferPool.freeAll();

    BaseType::shutdown();
  }

public:
  template <typename... Args>
  GenericBufferInterface *newBufferObject(Args &&...args)
  {
    void *memory = nullptr;
    {
      OSSpinlockScopedLock lock{bufferPoolGuard};
      memory = bufferPool.acquire();
    }
    // The constructor may kick off additional buffer allocations or frees so it can not be done
    // under the locked bufferPoolGuard
    auto buffer = ::new (memory) GenericBufferInterface(eastl::forward<Args>(args)...);
    if (!buffer->getDeviceBuffer() && !buffer->isStreamBuffer())
    {
      buffer->destroy();
      buffer = nullptr;
    }
    return buffer;
  }

  void deleteBufferObject(GenericBufferInterface *buffer)
  {
    // Have to destruct here to prevent possible recursive locking.
    buffer->~GenericBufferInterface();
    OSSpinlockScopedLock lock{bufferPoolGuard};
    bufferPool.release(buffer);
  }

  template <typename T>
  void visitBufferObjects(T clb)
  {
    OSSpinlockScopedLock lock{bufferPoolGuard};
    bufferPool.iterateAllocated(clb);
  }

  void reserveBufferObjects(size_t size)
  {
    OSSpinlockScopedLock lock{bufferPoolGuard};
    bufferPool.reserve(size);
  }

  size_t getBufferObjectCapacity()
  {
    OSSpinlockScopedLock lock{bufferPoolGuard};
    return bufferPool.capacity();
  }

  size_t getActiveBufferObjectCount()
  {
    OSSpinlockScopedLock lock{bufferPoolGuard};
    return bufferPool.size();
  }
};

class TextureObjectProvider : public BufferObjectProvider
{
  using BaseType = BufferObjectProvider;

protected:
  struct PendingForCompletedFrameData : BaseType::PendingForCompletedFrameData
  {
    eastl::vector<TextureInterfaceBase *> freedTextureObjects;
  };

  ContainerMutexWrapper<ObjectPool<TextureInterfaceBase>, OSSpinlock> texturePool;

  void shutdown()
  {
    auto poolAccess = texturePool.access();
    auto sz = poolAccess->size();
    G_ASSERTF(0 == sz, "DX12: Shutdown without destroying all textures, there are still %u textures alive!", sz);
#if DAGOR_DBGLEVEL > 0
    poolAccess->iterateAllocated([](auto tex) { G_ASSERTF(false, "DX12: Texture <%s> still alive!", tex->getResName()); });
#endif
    G_UNUSED(sz);
    poolAccess->freeAll();

    BaseType::shutdown();
  }

  void completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
  {
    if (!data.freedTextureObjects.empty())
    {
      eastl::sort(begin(data.freedTextureObjects), end(data.freedTextureObjects));
      texturePool.access()->free(begin(data.freedTextureObjects), end(data.freedTextureObjects));
      data.freedTextureObjects.clear();
    }
    BaseType::completeFrameExecution(info, data);
  }

public:
  template <typename... Args>
  TextureInterfaceBase *newTextureObject(Args &&...args)
  {
    return texturePool.access()->allocate(eastl::forward<Args>(args)...);
  }

  template <typename T>
  void visitTextureObjects(T clb)
  {
    texturePool.access()->iterateAllocated(clb);
  }

  void deleteTextureObjectOnFrameCompletion(TextureInterfaceBase *texture)
  {
    accessRecodingPendingFrameCompletion<PendingForCompletedFrameData>(
      [=](auto &data) { data.freedTextureObjects.push_back(texture); });
  }

  void reserveTextureObjects(size_t count) { texturePool.access()->reserve(count); }

  size_t getTextureObjectCapacity() { return texturePool.access()->capacity(); }

  size_t getActiveTextureObjectCount() { return texturePool.access()->size(); }
};

class ImageObjectProvider : public TextureObjectProvider
{
  using BaseType = TextureObjectProvider;

protected:
  ContainerMutexWrapper<ObjectPool<Image>, OSSpinlock> imageObjectPool;

  ImageObjectProvider() = default;
  ~ImageObjectProvider() = default;
  ImageObjectProvider(const ImageObjectProvider &) = delete;
  ImageObjectProvider &operator=(const ImageObjectProvider &) = delete;
  ImageObjectProvider(ImageObjectProvider &&) = delete;
  ImageObjectProvider &operator=(ImageObjectProvider &&) = delete;

  void shutdown()
  {
    imageObjectPool.access()->freeAll();
    BaseType::shutdown();
  }

  void preRecovery()
  {
    imageObjectPool.access()->freeAll();
    BaseType::preRecovery();
  }

  template <typename... Args>
  Image *newImageObject(Args &&...args)
  {
    return imageObjectPool.access()->allocate(eastl::forward<Args>(args)...);
  }

  void deleteImageObject(Image *image) { imageObjectPool.access()->free(image); }

  void deleteImageObjects(eastl::span<Image *> images)
  {
    auto imageObjectPoolAccess = imageObjectPool.access();
    for (auto image : images)
    {
      imageObjectPoolAccess->free(image);
    }
  }

public:
  template <typename T>
  void visitImageObjects(T &&clb)
  {
    imageObjectPool.access()->iterateAllocated(eastl::forward<T>(clb));
  }

  bool isImageAlive(Image *img) { return imageObjectPool.access()->isAllocated(img); }
};

} // namespace resource_manager
} // namespace drv3d_dx12
