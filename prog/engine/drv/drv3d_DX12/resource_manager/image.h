#pragma once

#include <dag/dag_vector.h>
#include <supp/dag_comPtr.h>
#include <osApiWrappers/dag_spinlock.h>

#include "driver.h"
#include "constants.h"
#include "extents.h"
#include "image_view_state.h"
#include "resource_memory.h"
#include "container_mutex_wrapper.h"
#include "image_global_subresource_id.h"
#include "texture_subresource_util.h"

#if DX12_USE_ESRAM
#include "resource_manager/esram_resource_xbox.h"
#endif

#if _TARGET_XBOX
#include "resource_manager/texture_access_computer_xbox.h"
#endif


namespace drv3d_dx12
{

#define DX12_IMAGE_DEBUG_NAMES 1
class Image
{
public:
  using ViewInfo = ImageViewInfo;
#if _TARGET_XBOX
  using TexAccessComputer = TextureAccessComputer::NativeType;
#endif

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
  uint8_t msaaLevel = 0;

  D3D12_RESOURCE_DIMENSION imageType = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  ArrayLayerCount layerCount{};
  MipMapCount mipLevels{};
  FormatStore format{};
  ExtendedImageGlobalSubresouceId globalSubResBase;
#if _TARGET_XBOX
  TextureAccessComputer textureAccessComputer;
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
  TexAccessComputer *getAccessComputer()
  {
    if (!textureAccessComputer)
    {
      D3D12_RESOURCE_DESC desc = image->GetDesc();
      textureAccessComputer.initialize(desc, textureLayout);
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

  bool isMultisampled() const { return msaaLevel > 0; }
  uint8_t getMsaaLevel() const { return msaaLevel; }

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
    Extent3D ext, MipMapCount levels, ArrayLayerCount layers, ImageGlobalSubresouceId sub_res_base, uint8_t msaa_level) :
    memory{mem},
    msaaLevel{msaa_level},
    imageType{type},
#ifdef _TARGET_XBOX
    textureLayout{layout},
#endif
    layerCount{layers},
    image{eastl::move(img)},
    format{fmt},
    extent{ext},
    mipLevels{levels},
    globalSubResBase{ExtendedImageGlobalSubresouceId::make(sub_res_base)}
  {
    G_UNUSED(layout);
  }

#if DX12_USE_ESRAM
  Image(ResourceMemory mem, ComPtr<ID3D12Resource> img, D3D12_RESOURCE_DIMENSION type, FormatStore fmt, Extent3D ext,
    MipMapCount levels, ArrayLayerCount layers, ImageGlobalSubresouceId sub_res_base, const EsramResource &esram_resource,
    uint8_t msaa_level) :
    memory{mem},
    msaaLevel{msaa_level},
    imageType{type},
    layerCount{layers},
    image{eastl::move(img)},
    format{fmt},
    extent{ext},
    mipLevels{levels},
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
      default: DAG_FATAL("Unreachable switch case"); return D3D_SRV_DIMENSION_TEXTURE2D;
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

} // namespace drv3d_dx12