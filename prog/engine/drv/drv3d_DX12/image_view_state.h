// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "bitfield.h"
#include "format_store.h"

#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_info.h>


namespace drv3d_dx12
{

BEGIN_BITFIELD_TYPE(ImageViewState, uint64_t)
  enum Type
  {
    INVALID, // 0!
    SRV,
    UAV,
    RTV,
    DSV_RW,
    DSV_CONST
  };
  enum
  {
    WORD_SIZE = sizeof(uint64_t) * 8,
    SAMPLE_STENCIL_BITS = 1,
    SAMPLE_STENCIL_SHIFT = 0,
    TYPE_BITS = 3,
    TYPE_SHIFT = SAMPLE_STENCIL_BITS + SAMPLE_STENCIL_SHIFT,
    IS_CUBEMAP_BITS = 1,
    IS_CUBEMAP_SHIFT = TYPE_BITS + TYPE_SHIFT,
    IS_ARRAY_BITS = 1,
    IS_ARRAY_SHIFT = IS_CUBEMAP_BITS + IS_CUBEMAP_SHIFT,
    FORMAT_BITS = FormatStore::BITS + 1,
    FORMAT_SHIFT = IS_ARRAY_SHIFT + IS_ARRAY_BITS,
    MIPMAP_OFFSET_BITS = BitsNeeded<15>::VALUE,
    MIPMAP_OFFSET_SHIFT = FORMAT_SHIFT + FORMAT_BITS,
    MIPMAP_RANGE_OFFSET = 1,
    MIPMAP_RANGE_BITS = BitsNeeded<16 - MIPMAP_RANGE_OFFSET>::VALUE,
    MIPMAP_RANGE_SHIFT = MIPMAP_OFFSET_SHIFT + MIPMAP_OFFSET_BITS,
    // automatic assign left over space to array range def
    ARRAY_DATA_SIZE = WORD_SIZE - MIPMAP_RANGE_SHIFT - MIPMAP_RANGE_BITS,
    ARRAY_OFFSET_BITS = (ARRAY_DATA_SIZE / 2) + (ARRAY_DATA_SIZE % 2),
    ARRAY_OFFSET_SHIFT = MIPMAP_RANGE_SHIFT + MIPMAP_RANGE_BITS,
    ARRAY_RANGE_OFFSET = 1,
    ARRAY_RANGE_BITS = ARRAY_DATA_SIZE / 2,
    ARRAY_RANGE_SHIFT = (ARRAY_OFFSET_SHIFT + ARRAY_OFFSET_BITS)
  };
  ADD_BITFIELD_MEMBER(sampleStencil, SAMPLE_STENCIL_SHIFT, SAMPLE_STENCIL_BITS)
  ADD_BITFIELD_MEMBER(type, TYPE_SHIFT, TYPE_BITS)
  ADD_BITFIELD_MEMBER(isCubemap, IS_CUBEMAP_SHIFT, IS_CUBEMAP_BITS)
  ADD_BITFIELD_MEMBER(isArray, IS_ARRAY_SHIFT, IS_ARRAY_BITS)
  ADD_BITFIELD_MEMBER(format, FORMAT_SHIFT, FORMAT_BITS)
  ADD_BITFIELD_MEMBER(mipmapOffset, MIPMAP_OFFSET_SHIFT, MIPMAP_OFFSET_BITS);
  ADD_BITFIELD_MEMBER(mipmapRange, MIPMAP_RANGE_SHIFT, MIPMAP_RANGE_BITS)
  ADD_BITFIELD_MEMBER(arrayOffset, ARRAY_OFFSET_SHIFT, ARRAY_OFFSET_BITS)
  ADD_BITFIELD_MEMBER(arrayRange, ARRAY_RANGE_SHIFT, ARRAY_RANGE_BITS)
  bool isValid() const
  {
    return uint64_t(*this) != 0;
  } // since type can't be 0/UNKNOWN, all bits can't be 0. comparing whole machine word is faster than extract type
  explicit operator bool() const { return isValid(); }
  void setType(Type tp) { type = tp; }
  Type getType() const { return static_cast<Type>(static_cast<uint32_t>(type)); }
  void setRTV() { setType(RTV); }
  void setDSV(bool as_const) { setType(as_const ? DSV_CONST : DSV_RW); }
  void setSRV() { setType(SRV); }
  void setUAV() { setType(UAV); }
  bool isRTV() const { return static_cast<uint32_t>(type) == RTV; }
  bool isDSV() const { return static_cast<uint32_t>(type) == DSV_RW || static_cast<uint32_t>(type) == DSV_CONST; }
  bool isSRV() const { return static_cast<uint32_t>(type) == SRV; }
  bool isUAV() const { return static_cast<uint32_t>(type) == UAV; }

  // TODO check cube/array handling

  // TODO: is d24/d32 always planar?
  D3D12_SHADER_RESOURCE_VIEW_DESC asSRVDesc(D3D12_RESOURCE_DIMENSION dim, bool is_multisampled) const
  {
    D3D12_SHADER_RESOURCE_VIEW_DESC result;
    const auto fmt = getFormat();
    G_ASSERT(!fmt.isDepth() || !is_multisampled || d3d::get_driver_desc().caps.hasReadMultisampledDepth);
    result.Format = fmt.asDxGiFormat();
    uint32_t planeSlice = 0;
    if (DXGI_FORMAT_D24_UNORM_S8_UINT == result.Format)
    {
      if (0 == sampleStencil)
      {
        result.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
      }
      else
      {
        result.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
        planeSlice = fmt.getPlanes().count() > 1 ? 1 : 0;
      }
    }
    else if (DXGI_FORMAT_D32_FLOAT_S8X24_UINT == result.Format)
    {
      if (0 == sampleStencil)
      {
        result.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
      }
      else
      {
        result.Format = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
        planeSlice = fmt.getPlanes().count() > 1 ? 1 : 0;
      }
    }
    else if (DXGI_FORMAT_D16_UNORM == result.Format)
    {
      result.Format = DXGI_FORMAT_R16_UNORM;
    }
    else if (DXGI_FORMAT_D32_FLOAT == result.Format)
    {
      result.Format = DXGI_FORMAT_R32_FLOAT;
    }
    result.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    switch (dim)
    {
      case D3D12_RESOURCE_DIMENSION_BUFFER:
      case D3D12_RESOURCE_DIMENSION_UNKNOWN: DAG_FATAL("Usage error!"); return {};
      case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (isArray)
        {
          result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
          auto &target = result.Texture1DArray;
          target.MostDetailedMip = getMipBase().index();
          target.MipLevels = getMipCount();
          target.FirstArraySlice = getArrayBase().index();
          target.ArraySize = getArrayCount();
          target.ResourceMinLODClamp = 0.f;
        }
        else
        {
          result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
          auto &target = result.Texture1D;
          target.MostDetailedMip = getMipBase().index();
          target.MipLevels = getMipCount();
          target.ResourceMinLODClamp = 0.f;
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (isCubemap)
        {
          if (isArray)
          {
            result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            auto &target = result.TextureCubeArray;
            target.MostDetailedMip = getMipBase().index();
            target.MipLevels = getMipCount();
            target.First2DArrayFace = getArrayBase().index();
            target.NumCubes = getArrayCount() / 6;
            target.ResourceMinLODClamp = 0.f;
          }
          else
          {
            result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            auto &target = result.TextureCube;
            target.MostDetailedMip = getMipBase().index();
            target.MipLevels = getMipCount();
            target.ResourceMinLODClamp = 0.f;
          }
        }
        else
        {
          if (isArray)
          {
            if (!is_multisampled)
            {
              result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
              auto &target = result.Texture2DArray;
              target.MostDetailedMip = getMipBase().index();
              target.MipLevels = getMipCount();
              target.FirstArraySlice = getArrayBase().index();
              target.ArraySize = getArrayCount();
              target.PlaneSlice = planeSlice;
              target.ResourceMinLODClamp = 0.f;
            }
            else
            {
              result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
              auto &target = result.Texture2DMSArray;
              target.FirstArraySlice = getArrayBase().index();
              target.ArraySize = getArrayCount();
            }
          }
          else
          {
            if (!is_multisampled)
            {
              result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
              auto &target = result.Texture2D;
              target.MostDetailedMip = getMipBase().index();
              target.MipLevels = getMipCount();
              target.PlaneSlice = planeSlice;
              target.ResourceMinLODClamp = 0.f;
            }
            else
            {
              result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
              auto &target = result.Texture2DMS;
              G_UNUSED(target);
            }
          }
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        result.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        auto &target = result.Texture3D;
        target.MostDetailedMip = getMipBase().index();
        target.MipLevels = getMipCount();
        target.ResourceMinLODClamp = 0.f;
        break;
    }
    return result;
  }

  D3D12_UNORDERED_ACCESS_VIEW_DESC asUAVDesc(D3D12_RESOURCE_DIMENSION dim) const
  {
    D3D12_UNORDERED_ACCESS_VIEW_DESC result;
    result.Format = getFormat().asDxGiFormat();
    switch (dim)
    {
      case D3D12_RESOURCE_DIMENSION_BUFFER:
      case D3D12_RESOURCE_DIMENSION_UNKNOWN: DAG_FATAL("Usage error!"); return {};
      case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (isArray)
        {
          result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
          auto &target = result.Texture1DArray;
          target.MipSlice = getMipBase().index();
          target.FirstArraySlice = getArrayBase().index();
          target.ArraySize = getArrayCount();
        }
        else
        {
          result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
          auto &target = result.Texture1D;
          target.MipSlice = getMipBase().index();
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        // Array and cube are the same for UAV
        if (isArray || isCubemap)
        {
          result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
          auto &target = result.Texture2DArray;
          target.MipSlice = getMipBase().index();
          target.FirstArraySlice = getArrayBase().index();
          target.ArraySize = getArrayCount();
          target.PlaneSlice = 0;
        }
        else
        {
          result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
          auto &target = result.Texture2D;
          target.MipSlice = getMipBase().index();
          target.PlaneSlice = 0;
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        result.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        auto &target = result.Texture3D;
        target.MipSlice = getMipBase().index();
        target.FirstWSlice = getArrayBase().index();
        target.WSize = getArrayCount();
        break;
    }
    return result;
  }

  D3D12_RENDER_TARGET_VIEW_DESC asRTVDesc(D3D12_RESOURCE_DIMENSION dim, bool is_multisampled) const
  {
    D3D12_RENDER_TARGET_VIEW_DESC result;
    result.Format = getFormat().asDxGiFormat();
    switch (dim)
    {
      case D3D12_RESOURCE_DIMENSION_BUFFER:
      case D3D12_RESOURCE_DIMENSION_UNKNOWN: DAG_FATAL("Usage error!"); return {};
      case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (isArray)
        {
          result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
          auto &target = result.Texture1DArray;
          target.MipSlice = getMipBase().index();
          target.FirstArraySlice = getArrayBase().index();
          target.ArraySize = getArrayCount();
        }
        else
        {
          result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
          auto &target = result.Texture1D;
          target.MipSlice = getMipBase().index();
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (isArray || isCubemap)
        {
          result.ViewDimension = is_multisampled ? D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY : D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
          if (!is_multisampled)
          {
            auto &target = result.Texture2DArray;
            target.MipSlice = getMipBase().index();
            target.FirstArraySlice = getArrayBase().index();
            target.ArraySize = getArrayCount();
            target.PlaneSlice = 0;
          }
          else
          {
            auto &target = result.Texture2DMSArray;
            target.ArraySize = getArrayCount();
            target.FirstArraySlice = getArrayBase().index();
          }
        }
        else
        {
          result.ViewDimension = is_multisampled ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;
          if (!is_multisampled)
          {
            auto &target = result.Texture2D;
            target.MipSlice = getMipBase().index();
            target.PlaneSlice = 0;
          }
          else
          {
            auto &target = result.Texture2DMS;
            G_UNUSED(target);
          }
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        result.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        auto &target = result.Texture3D;
        target.MipSlice = getMipBase().index();
        target.FirstWSlice = getArrayBase().index();
        target.WSize = getArrayCount();
        break;
    }

    return result;
  }

  D3D12_DEPTH_STENCIL_VIEW_DESC asDSVDesc(D3D12_RESOURCE_DIMENSION dim, bool is_multisampled) const
  {
    D3D12_DEPTH_STENCIL_VIEW_DESC result;
    auto fmt = getFormat();
    result.Format = fmt.asDxGiFormat();
    result.Flags = D3D12_DSV_FLAG_NONE;
    if (getType() == DSV_CONST)
    {
      if (fmt.isDepth())
        result.Flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
      if (fmt.isStencil())
        result.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;
    }
    switch (dim)
    {
      case D3D12_RESOURCE_DIMENSION_BUFFER:
      case D3D12_RESOURCE_DIMENSION_UNKNOWN: DAG_FATAL("Usage error!"); return {};
      case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        if (isArray)
        {
          result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
          auto &target = result.Texture1DArray;
          target.MipSlice = getMipBase().index();
          target.FirstArraySlice = getArrayBase().index();
          target.ArraySize = getArrayCount();
        }
        else
        {
          result.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
          auto &target = result.Texture1D;
          target.MipSlice = getMipBase().index();
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (isArray || isCubemap)
        {
          result.ViewDimension = is_multisampled ? D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY : D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
          if (!is_multisampled)
          {
            auto &target = result.Texture2DArray;
            target.MipSlice = getMipBase().index();
            target.FirstArraySlice = getArrayBase().index();
            target.ArraySize = getArrayCount();
          }
          else
          {
            auto &target = result.Texture2DMSArray;
            target.FirstArraySlice = getArrayBase().index();
            target.ArraySize = getArrayCount();
          }
        }
        else
        {
          result.ViewDimension = is_multisampled ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
          if (!is_multisampled)
          {
            auto &target = result.Texture2D;
            target.MipSlice = getMipBase().index();
          }
          else
          {
            auto &target = result.Texture2DMS;
            G_UNUSED(target);
          }
        }
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE3D: DAG_FATAL("DX12: Volume depth stencil view not supported"); break;
    }
    return result;
  }

  void setFormat(FormatStore fmt) { format = fmt.index; }
  FormatStore getFormat() const { return FormatStore(format); }
  void setMipBase(MipMapIndex u) { mipmapOffset = u.index(); }
  MipMapIndex getMipBase() const { return MipMapIndex::make(mipmapOffset); }
  void setMipCount(uint8_t u) { mipmapRange = u - MIPMAP_RANGE_OFFSET; }
  uint8_t getMipCount() const { return MIPMAP_RANGE_OFFSET + mipmapRange; }
  void setSingleMipMapRange(MipMapIndex index)
  {
    setMipBase(index);
    G_STATIC_ASSERT(1 == MIPMAP_RANGE_OFFSET);
    mipmapRange = 0;
  }
  void setMipMapRange(MipMapIndex index, uint32_t count)
  {
    setMipBase(index);
    setMipCount(count);
  }
  void setMipMapRange(MipMapRange range)
  {
    setMipBase(range.front());
    setMipCount(range.size());
  }
  MipMapRange getMipRange() const { return MipMapRange::make(getMipBase(), getMipCount()); }
  void setArrayBase(ArrayLayerIndex u) { arrayOffset = u.index(); }
  ArrayLayerIndex getArrayBase() const { return ArrayLayerIndex::make(arrayOffset); }
  void setArrayCount(uint16_t u) { arrayRange = u - ARRAY_RANGE_OFFSET; }
  uint16_t getArrayCount() const { return ARRAY_RANGE_OFFSET + (uint32_t)arrayRange; }
  void setArrayRange(ArrayLayerRange range)
  {
    setArrayBase(range.front());
    setArrayCount(range.size());
  }
  void setSingleArrayRange(ArrayLayerIndex index)
  {
    setArrayBase(index);
    G_STATIC_ASSERT(1 == ARRAY_RANGE_OFFSET);
    arrayRange = 0;
  }
  ArrayLayerRange getArrayRange() const { return ArrayLayerRange::make(getArrayBase(), getArrayCount()); }
  void setSingleDepthLayer(uint16_t base)
  {
    setArrayBase(ArrayLayerIndex::make(base));
    setArrayCount(1);
  }
  void setDepthLayerRange(uint16_t base, uint16_t count)
  {
    setArrayBase(ArrayLayerIndex::make(base));
    setArrayCount(count);
  }
  FormatPlaneIndex getPlaneIndex() const
  {
    return FormatPlaneIndex::make((sampleStencil && getFormat().getPlanes().count() > 1) ? 1 : 0);
  }
  template <typename T>
  // NOTE: does not take plane index into account
  void iterateSubresources(D3D12_RESOURCE_DIMENSION dim, MipMapCount mip_per_array, T clb) const
  {
    if (D3D12_RESOURCE_DIMENSION_TEXTURE3D == dim)
    {
      for (auto m : getMipRange())
      {
        clb(SubresourceIndex::make(m.index()));
      }
    }
    else
    {
      for (auto a : getArrayRange())
      {
        for (auto m : getMipRange())
        {
          clb(calculate_subresource_index(m, a, mip_per_array));
        }
      }
    }
  }
END_BITFIELD_TYPE()

inline bool operator==(ImageViewState l, ImageViewState r) { return l.wrapper.value == r.wrapper.value; }
inline bool operator!=(ImageViewState l, ImageViewState r) { return l.wrapper.value != r.wrapper.value; }
inline bool operator<(ImageViewState l, ImageViewState r) { return l.wrapper.value < r.wrapper.value; }

struct ImageViewInfo
{
  D3D12_CPU_DESCRIPTOR_HANDLE handle;
  ImageViewState state;
};

} // namespace drv3d_dx12
