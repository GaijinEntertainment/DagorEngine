// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "basic_components.h"
#include "image.h"
#include <buffer.h>
#include <device_memory_class.h>
#include <extents.h>
#include <format_store.h>
#include <image_view_state.h>
#include <resource_memory.h>
#include <texture.h>
#include <texture_subresource_util.h>

#include <dag/dag_vector.h>
#include <generic/dag_objectPool.h>
#include <math/dag_adjpow2.h>
#include <osApiWrappers/dag_spinlock.h>
#include <supp/dag_comPtr.h>


namespace drv3d_dx12
{

inline uint64_t calculate_texture_alignment(uint64_t width, uint32_t height, uint32_t depth, uint32_t samples,
  D3D12_TEXTURE_LAYOUT layout, D3D12_RESOURCE_FLAGS flags, drv3d_dx12::FormatStore format)
{
  if (D3D12_TEXTURE_LAYOUT_UNKNOWN != layout)
  {
    if (samples > 1)
    {
      return D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
    }
    else
    {
      return D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    }
  }

  if ((1 == samples) && ((D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) & flags))
  {
    return D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  }

  uint32_t blockSizeX = 1, blockSizeY = 1;
  auto bytesPerBlock = format.getBytesPerPixelBlock(&blockSizeX, &blockSizeY);
  const uint32_t textureWidthInBlocks = (width + blockSizeX - 1) / blockSizeX;
  const uint32_t textureHeightInBlocks = (height + blockSizeY - 1) / blockSizeY;

  const uint32_t TILE_MEM_SIZE = 4 * 1024;
  const uint32_t blocksInTile = TILE_MEM_SIZE / bytesPerBlock;
  // MSDN documentation says about "near-equilateral" size for the tile
  const uint32_t blocksInTileX = get_bigger_pow2(sqrt(blocksInTile));
  const uint32_t blocksInTileY = get_bigger_pow2(blocksInTile / blocksInTileX);
  const uint32_t MAX_TILES_COUNT_FOR_SMALL_RES = 16;
  const uint32_t tilesCount = ((textureWidthInBlocks + blocksInTileX - 1) / blocksInTileX) *
                              ((textureHeightInBlocks + blocksInTileY - 1) / blocksInTileY) * depth;
  // This check is neccessary according to debug layer and dx12 documentation:
  // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_desc#alignment
  const bool smallAmountOfTiles = tilesCount <= MAX_TILES_COUNT_FOR_SMALL_RES;

  if (samples > 1)
  {
    if (smallAmountOfTiles)
    {
      return D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
    }
    else
    {
      return D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
    }
  }
  else
  {
    if (smallAmountOfTiles)
    {
      return D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
    }
    else
    {
      return D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    }
  }
}

// size is not important here
struct ImageInfo
{
  D3D12_RESOURCE_DIMENSION type = D3D12_RESOURCE_DIMENSION_UNKNOWN;
  D3D12_RESOURCE_FLAGS usage = D3D12_RESOURCE_FLAG_NONE;
  Extent3D size = {};
  ArrayLayerCount arrays;
  MipMapCount mips;
  FormatStore format;
  D3D12_TEXTURE_LAYOUT memoryLayout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  DeviceMemoryClass memoryClass = DeviceMemoryClass::INVALID;
  bool allocateSubresourceIDs = false;
  DXGI_SAMPLE_DESC sampleDesc = {1, 0};

  SubresourceCount getSubResourceCount() const { return SubresourcePerFormatPlaneCount::make(mips, arrays) * format.getPlanes(); }

  D3D12_RESOURCE_DESC asDesc() const
  {
    D3D12_RESOURCE_DESC desc;
    desc.SampleDesc = sampleDesc;
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
      case D3D12_RESOURCE_DIMENSION_BUFFER: DAG_FATAL("DX12: Invalid texture dimension"); return desc;
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
#if DAGOR_DBGLEVEL > 0
    bufferPool.iterateAllocated([](auto buffer) { logwarn("DX12: Buffer <%s> still alive!", buffer->getBufName()); });
#endif
    G_ASSERTF(0 == sz, "DX12: Shutdown without destroying all buffers, there are still %u buffers alive!", sz);
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

  /// visits all buffers with explicit(ly named) interface that T has to implement
  template <typename T>
  void visitBufferObjectsExplicit(T clb)
  {
    OSSpinlockScopedLock lock{bufferPoolGuard};
    clb.beginVisit(bufferPool.size());
    bufferPool.iterateAllocated([&clb](auto buffer) { clb.visitBuffer(buffer); });
    clb.endVisit();
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
    dag::Vector<TextureInterfaceBase *> freedTextureObjects;
  };

  ContainerMutexWrapper<ObjectPool<TextureInterfaceBase>, OSSpinlock> texturePool;

  void shutdown()
  {
    auto poolAccess = texturePool.access();
    auto sz = poolAccess->size();
#if DAGOR_DBGLEVEL > 0
    poolAccess->iterateAllocated([](auto tex) { logwarn("DX12: Texture <%s> still alive!", tex->getResName()); });
#endif
    G_ASSERTF(0 == sz, "DX12: Shutdown without destroying all textures, there are still %u textures alive!", sz);
    G_UNUSED(sz);
    poolAccess->freeAll();

    BaseType::shutdown();
  }

  void preRecovery()
  {
    texturePool.access()->iterateAllocated([](auto tex) { tex->preRecovery(); });
    BaseType::preRecovery();
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

  /// visits all textures with explicit(ly named) interface that T has to implement
  template <typename T>
  void visitTextureObjectsExplicit(T clb)
  {
    auto poolAccess = texturePool.access();
    clb.beginVisit();
    poolAccess->iterateAllocated([&clb](auto tex) { clb.visitTexture(tex); });
    clb.endVisit();
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

  /// visits all images with explicit(ly named) interface that T has to implement
  template <typename T>
  void visitImageObjectsExplicit(T &&clb)
  {
    auto poolAccess = imageObjectPool.access();
    clb.beginVisit();
    poolAccess->iterateAllocated([&clb](auto image) { clb.visitTexture(image); });
    clb.endVisit();
  }

  bool isImageAlive(Image *img) { return imageObjectPool.access()->isAllocated(img); }
};

} // namespace resource_manager
} // namespace drv3d_dx12
