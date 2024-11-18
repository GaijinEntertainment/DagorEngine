// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <constants.h>
#include <container_mutex_wrapper.h>
#include <driver.h>
#include <extents.h>
#include <image_global_subresource_id.h>
#include <image_view_state.h>
#include <resource_memory.h>
#include <texture_subresource_util.h>

#include <dag/dag_vector.h>
#include <osApiWrappers/dag_spinlock.h>
#include <supp/dag_comPtr.h>

#if DX12_USE_ESRAM
#include "esram_resource_xbox.h"
#endif

#if _TARGET_XBOX
#include "texture_access_computer_xbox.h"
#endif

#define DX12_IMAGE_DEBUG_NAMES 1

namespace drv3d_dx12
{
inline void printDesc(const D3D12_RESOURCE_DESC &desc)
{
  logdbg("DX12: desc.Dimension = %s", to_string(desc.Dimension));
  logdbg("DX12: desc.Alignment = %llu", desc.Alignment);
  logdbg("DX12: desc.Width = %llu", desc.Width);
  logdbg("DX12: desc.Height = %u", desc.Height);
  logdbg("DX12: desc.DepthOrArraySize = %u", desc.DepthOrArraySize);
  logdbg("DX12: desc.MipLevels = %u", desc.MipLevels);
  logdbg("DX12: desc.Format = %u", dxgi_format_name(desc.Format));
  logdbg("DX12: desc.SampleDesc.Count = %u", desc.SampleDesc.Count);
  logdbg("DX12: desc.SampleDesc.Quality = %u", desc.SampleDesc.Quality);
  logdbg("DX12: desc.Layout = %u", desc.Layout);
  logdbg("DX12: desc.Flags = %08X", desc.Flags);
}

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
#if DAGOR_DBGLEVEL > 0
  void dbgOnImageValidationFailed(ImageViewState image_view_state, const D3D12_RESOURCE_DESC &desc)
  {
    logdbg("DX12: image.debugName = %s", *debugName.access());
    logdbg("DX12: image.mipLevels = %d", mipLevels.count());
    logdbg("DX12: image.layerCount = %d", layerCount);
    logdbg("DX12: image.dimension = %d", imageType);
    logdbg("DX12: image.extent = (%d, %d, %d)", extent.width, extent.height, extent.depth);
    logdbg("DX12: view.mipBase = %d", image_view_state.getMipBase());
    logdbg("DX12: view.mipCount = %d", image_view_state.getMipCount());
    logdbg("DX12: view.arrayBase = %d", image_view_state.getArrayBase());
    logdbg("DX12: view.arrayCount = %d", image_view_state.getArrayCount());
    printDesc(desc);
    G_ASSERT_FAIL("DX12: Image validation failed, please attach logs and dump to report");
  }

  void dbgCheckImageValidationCondition(bool condition, const char *str, ImageViewState image_view_state,
    const D3D12_RESOURCE_DESC &desc)
  {
    if (condition)
      return;
    logerr("DX12: Image validation info failed (%s). Logging debug info...", str);
    dbgOnImageValidationFailed(image_view_state, desc);
  }
#endif

  void dbgValidateImageViewStateCompatibility([[maybe_unused]] ImageViewState image_view_state)
  {
#if DAGOR_DBGLEVEL > 0
    G_ASSERTF(image, "DX12: Image view validation failed, image is not presented (tex: %s)", *debugName.access());
    const auto desc = image->GetDesc();
    dbgCheckImageValidationCondition(image_view_state.getMipBase() + image_view_state.getMipCount() <= mipLevels, "wrong mip range",
      image_view_state, desc);
    dbgCheckImageValidationCondition(image_view_state.getArrayBase() + image_view_state.getArrayCount() <= layerCount,
      "wrong layers range", image_view_state, desc);
    dbgCheckImageValidationCondition(desc.DepthOrArraySize == max((uint32_t)layerCount.count(), extent.depth),
      "wrong layers count in desc", image_view_state, desc);
    dbgCheckImageValidationCondition(desc.MipLevels == mipLevels.count(), "wrong mips count in desc", image_view_state, desc);
    dbgCheckImageValidationCondition(desc.Dimension == imageType, "wrong dimension", image_view_state, desc);
#endif
  }
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