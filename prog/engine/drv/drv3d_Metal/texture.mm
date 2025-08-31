// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>
#include "render.h"
#include "drv_log_defs.h"
#include "drv_assert_defs.h"

#include <vecmath/dag_vecMath_common.h>
#include "textureloader.h"

#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <AvailabilityMacros.h>

#include <drv/3d/dag_info.h>

#include <osApiWrappers/dag_miscApi.h>
#include "pools.h"
#include <util/dag_string.h>
#include <generic/dag_sort.h>
#include "statStr.h"
#include <basetexture.h>
#include <validateUpdateSubRegion.h>

typedef drv3d_generic::ObjectProxyPtr<drv3d_metal::Texture::ApiTexture> TextureProxyPtr;

static drv3d_generic::ObjectPoolWithLock<TextureProxyPtr> g_textures("textures");

namespace drv3d_metal
{
  void clear_textures_pool_garbage() { g_textures.clearGarbage(); }
  static bool add_texture_to_list(drv3d_metal::Texture::ApiTexture *t)
  {
    TextureProxyPtr e;
    e.obj = t;
    t->tid = g_textures.safeAllocAndSet(e);
    return t->tid != drv3d_generic::BAD_HANDLE;
  }

  MTLPixelFormat Texture::format2Metal(uint32_t fmt)
  {
    switch (fmt)
    {
      case TEXFMT_A8R8G8B8:     return MTLPixelFormatBGRA8Unorm;
      case TEXFMT_A2R10G10B10:  return MTLPixelFormatRGB10A2Unorm;
      case TEXFMT_A2B10G10R10:  return MTLPixelFormatRGB10A2Unorm;
      case TEXFMT_A16B16G16R16:   return MTLPixelFormatRGBA16Unorm;
      case TEXFMT_A16B16G16R16F:  return MTLPixelFormatRGBA16Float;
      case TEXFMT_A32B32G32R32F:  return MTLPixelFormatRGBA32Float;
      case TEXFMT_G16R16:     return MTLPixelFormatRG16Unorm;
      case TEXFMT_G16R16F:    return MTLPixelFormatRG16Float;
      case TEXFMT_G32R32F:    return MTLPixelFormatRG32Float;
      case TEXFMT_R16F:       return MTLPixelFormatR16Float;
      case TEXFMT_R32F:       return MTLPixelFormatR32Float;
#if _TARGET_PC_MACOSX
      case TEXFMT_DXT1:       return MTLPixelFormatBC1_RGBA;
      case TEXFMT_DXT3:       return MTLPixelFormatBC2_RGBA;
      case TEXFMT_DXT5:       return MTLPixelFormatBC3_RGBA;
      case TEXFMT_ATI1N:      return MTLPixelFormatBC4_RUnorm;
      case TEXFMT_ATI2N:      return MTLPixelFormatBC5_RGUnorm;
      case TEXFMT_BC6H:       return MTLPixelFormatBC6H_RGBFloat;
      case TEXFMT_BC7:      return MTLPixelFormatBC7_RGBAUnorm;
#else
      case TEXFMT_ASTC4:          return MTLPixelFormatASTC_4x4_LDR;
      case TEXFMT_ASTC8:          return MTLPixelFormatASTC_8x8_LDR;
      case TEXFMT_ASTC12:         return MTLPixelFormatASTC_12x12_LDR;
#endif
      case TEXFMT_R32G32UI: return MTLPixelFormatRG32Uint;
      case TEXFMT_ETC2_RGBA: return MTLPixelFormatEAC_RGBA8;
      case TEXFMT_ETC2_RG: return MTLPixelFormatEAC_RG11Unorm;
      case TEXFMT_L16:      return MTLPixelFormatR16Unorm;
      case TEXFMT_A8:       return MTLPixelFormatA8Unorm;
      case TEXFMT_R8:       return MTLPixelFormatR8Unorm;
      case TEXFMT_A1R5G5B5: return MTLPixelFormatBGR5A1Unorm;
      case TEXFMT_A4R4G4B4: return MTLPixelFormatABGR4Unorm;
      case TEXFMT_R5G6B5: return MTLPixelFormatB5G6R5Unorm;
      case TEXFMT_A16B16G16R16S:  return MTLPixelFormatRGBA16Snorm;
      case TEXFMT_A16B16G16R16UI: return MTLPixelFormatRGBA16Uint;
      case TEXFMT_A32B32G32R32UI: return MTLPixelFormatRGBA32Uint;
      case TEXFMT_R8G8B8A8:     return MTLPixelFormatRGBA8Unorm;
      case TEXFMT_R32UI:      return MTLPixelFormatR32Uint;
      case TEXFMT_R32SI:      return MTLPixelFormatR32Sint;
      case TEXFMT_R8UI:       return MTLPixelFormatR8Uint;
      case TEXFMT_R16UI:      return MTLPixelFormatR16Uint;
      case TEXFMT_R11G11B10F:   return MTLPixelFormatRG11B10Float;
      case TEXFMT_R9G9B9E5:   return MTLPixelFormatRGB9E5Float;
      case TEXFMT_R8G8:       return MTLPixelFormatRG8Unorm;
      case TEXFMT_R8G8S:      return MTLPixelFormatRG8Snorm;
      case TEXFMT_DEPTH24:    return MTLPixelFormatDepth32Float_Stencil8;// MTLPixelFormatDepth24Unorm_Stencil8;
      case TEXFMT_DEPTH16: return MTLPixelFormatDepth16Unorm;
      case TEXFMT_DEPTH32:    return MTLPixelFormatDepth32Float;
      case TEXFMT_DEPTH32_S8:   return MTLPixelFormatDepth32Float_Stencil8;
    }

    DAG_FATAL("format2Metal unimplemented for format %i", fmt);

    return MTLPixelFormatInvalid;
  }

  static bool canAliasToUint(uint32_t fmt)
  {
    switch (fmt)
    {
      case TEXFMT_A8R8G8B8:
      case TEXFMT_A2R10G10B10:
      case TEXFMT_A2B10G10R10:
      case TEXFMT_G16R16:
      case TEXFMT_G16R16F:
      case TEXFMT_R32F:
      case TEXFMT_R8G8B8A8:
      case TEXFMT_R32UI:
      case TEXFMT_R32SI:
      case TEXFMT_R11G11B10F:
      case TEXFMT_R9G9B9E5:
      case TEXFMT_DEPTH32:
        return true;
    }
    return false;
  }

  MTLPixelFormat Texture::format2MetalsRGB(uint32_t fmt)
  {
    switch (fmt)
    {
#if _TARGET_PC_MACOSX
      case TEXFMT_DXT1:      return MTLPixelFormatBC1_RGBA_sRGB;
      case TEXFMT_DXT3:      return MTLPixelFormatBC2_RGBA_sRGB;
      case TEXFMT_DXT5:      return MTLPixelFormatBC3_RGBA_sRGB;
      case TEXFMT_BC7:       return MTLPixelFormatBC7_RGBAUnorm_sRGB;
#else
      case TEXFMT_ASTC4:     return MTLPixelFormatASTC_4x4_sRGB;
      case TEXFMT_ASTC8:     return MTLPixelFormatASTC_8x8_sRGB;
      case TEXFMT_ASTC12:    return MTLPixelFormatASTC_12x12_sRGB;
#endif
      case TEXFMT_A8R8G8B8:  return MTLPixelFormatBGRA8Unorm_sRGB;
      case TEXFMT_R8G8B8A8:  return MTLPixelFormatRGBA8Unorm_sRGB;

      case TEXFMT_ETC2_RGBA: return MTLPixelFormatEAC_RGBA8_sRGB;
    }

    return format2Metal(fmt);
  }

  int Texture::getMaxLevels(int width, int height)
  {
    int levels = 1;

    int wdt = width;
    int hgt = height;

    while (wdt != 1 || hgt != 1)
    {
      if (wdt > 1)
      {
        wdt = wdt >> 1;
      }

      if (hgt > 1)
      {
        hgt = hgt >> 1;
      }

      levels++;
    }

    return levels;
  }

  void Texture::getStride(uint32_t fmt, int width, int height, int level, int& widBytes, int& nBytes)
  {
    width = max(width >> level, 1);
    height = max(height >> level, 1);

    bool is_compressed = false;
    int block_size = 1, block_width = 1, block_height = 1;

    switch (fmt)
    {
      case TEXFMT_R8G8:
      case TEXFMT_R8G8S:
      case TEXFMT_R16F:
      case TEXFMT_L16:
      case TEXFMT_R16UI:
      {
        block_size = 2;
        break;
      }
      case TEXFMT_A1R5G5B5:
      case TEXFMT_A4R4G4B4:
      case TEXFMT_R5G6B5:
      {
        block_size = 2;
        break;
      }
      case TEXFMT_A8R8G8B8:
      case TEXFMT_R8G8B8A8:
      case TEXFMT_A2R10G10B10:
      case TEXFMT_A2B10G10R10:
      case TEXFMT_G16R16:
      case TEXFMT_G16R16F:
      case TEXFMT_R32F:
      case TEXFMT_R32UI:
      case TEXFMT_R32SI:
      case TEXFMT_R11G11B10F:
      case TEXFMT_R9G9B9E5:
      {
        block_size = 4;
        break;
      }
      case TEXFMT_A16B16G16R16:
      case TEXFMT_A16B16G16R16F:
      case TEXFMT_A16B16G16R16S:
      case TEXFMT_A16B16G16R16UI:
      case TEXFMT_R32G32UI:
      case TEXFMT_G32R32F:
      {
        block_size = 8;
        break;
      }
      case TEXFMT_A32B32G32R32F:
      case TEXFMT_A32B32G32R32UI:
      {
        block_size = 16;
        break;
      }
      case TEXFMT_DXT1:
      case TEXFMT_ATI1N:
      {
        is_compressed = true;
        block_size = 8;
        block_width = block_height = 4;
        break;
      }
      case TEXFMT_DXT3:
      case TEXFMT_DXT5:
      case TEXFMT_BC7:
      case TEXFMT_BC6H:
      case TEXFMT_ATI2N:
      case TEXFMT_ETC2_RGBA:
      case TEXFMT_ETC2_RG:
      case TEXFMT_ASTC4:
      {
        is_compressed = true;
        block_size = 16;
        block_width = block_height = 4;
        break;
      }
      case TEXFMT_ASTC8:
      {
        is_compressed = true;
        block_size = 16;
        block_width = block_height = 8;
        break;
      }
      case TEXFMT_ASTC12:
      {
        is_compressed = true;
        block_size = 16;
        block_width = block_height = 12;
        break;
      }
      case TEXFMT_A8:
      case TEXFMT_R8:
      {
        break;
      }
      case TEXFMT_R8UI:
      {
        block_size = 1;
        break;
      }
      case TEXFMT_DEPTH16:
      {
        block_size = 2;
        break;
      }
      case TEXFMT_DEPTH24:
      case TEXFMT_DEPTH32:
      {
        block_size = 4;
        break;
      }
      case TEXFMT_DEPTH32_S8:
      {
        block_size = 5;
        break;
      }
      default:
        DAG_FATAL("getStride unimplemented for format %i", fmt);
    }

    if (!is_compressed)
    {
      widBytes = width * block_size;
      nBytes = widBytes * height;
    }
    else
    {
      width = max(width, 4);
      height = max(height, 4);
      const int allignedWidth = (width + block_width - 1) / block_width;
      const int allignedHeight = (height + block_height - 1) / block_height;

      widBytes = allignedWidth * block_size;
      nBytes = allignedWidth * allignedHeight * block_size;
    }
  }

  bool Texture::isSameFormat(int cflg1, int cflg2)
  {
    if ((cflg1 & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) == (cflg2 & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) &&
       (cflg1 & TEXFMT_MASK) == (cflg2 & TEXFMT_MASK))
    {
      return true;
    }
    return false;
  }

  static void setTexName(id<MTLTexture> tex, const char* name)
  {
#if DAGOR_DBGLEVEL > 0
    if (tex)
      tex.label = [NSString stringWithFormat:@"%s %llu", name, render.frame];
#endif
  }

  Texture::ApiTexture::ApiTexture(Texture* base) : base(base)
  {
    MTLTextureDescriptor *pTexDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat : base->metal_format
          width : base->width
          height : base->height
          mipmapped : NO];

    MTLResourceOptions storage_mode = MTLResourceStorageModePrivate;
    MTLResourceOptions tracking_mode = render.manual_hazard_tracking ? MTLResourceHazardTrackingModeUntracked : MTLResourceHazardTrackingModeDefault;

    if (base->metal_format == MTLPixelFormatDepth32Float ||
        base->metal_format == MTLPixelFormatDepth32Float_Stencil8)
    {
      pTexDesc.storageMode = MTLStorageModePrivate;
      pTexDesc.resourceOptions = storage_mode | tracking_mode;
      pTexDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    }
    else if (base->cflg & TEXCF_UNORDERED)
    {
      pTexDesc.storageMode = MTLStorageModePrivate;
      pTexDesc.resourceOptions = storage_mode | tracking_mode;
      pTexDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget | MTLTextureUsageShaderWrite;
    }
    else if (base->cflg & TEXCF_RTARGET)
    {
      pTexDesc.storageMode = MTLStorageModePrivate;
      pTexDesc.resourceOptions = storage_mode | tracking_mode;
      pTexDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    }
    else
      pTexDesc.resourceOptions = tracking_mode;

    if (base->use_upload_buffer)
      pTexDesc.storageMode = MTLStorageModePrivate;

    pTexDesc.usage |= MTLTextureUsageShaderRead;

    bool as_uint = (base->cflg & TEXCF_UNORDERED) && canAliasToUint(base->base_format & TEXFMT_MASK);
    bool no_srgb = (base->cflg & TEXCF_UNORDERED) && (base->cflg & TEXCF_SRGBREAD) && format2Metal(base->base_format) != base->metal_format;
    if (base->metal_format == MTLPixelFormatDepth32Float_Stencil8 || base->metal_format != base->metal_rt_format || as_uint || no_srgb)
      pTexDesc.usage |= MTLTextureUsagePixelFormatView;

    if (base->memoryless)
      pTexDesc.sampleCount = 1; // base->samples;
    else
    {
      pTexDesc.sampleCount = base->samples;
      if (base->samples > 1)
        base->metal_type = MTLTextureType2DMultisample;
    }

    pTexDesc.mipmapLevelCount = base->mipLevels;
    pTexDesc.textureType = base->metal_type;

    if (base->type == D3DResourceType::ARRTEX || base->type == D3DResourceType::CUBEARRTEX)
    {
      pTexDesc.arrayLength = base->depth;
      pTexDesc.depth = 1;
    }
    else
    {
      G_ASSERT(base->type == D3DResourceType::VOLTEX || base->depth == 1);
      pTexDesc.depth = base->depth;
    }

    texture = [render.device newTextureWithDescriptor : pTexDesc];
    if (texture == nil)
      DAG_FATAL("Failed to allocate texture %s", base->getName());

    int slises = 1;
    if (base->type == D3DResourceType::ARRTEX || base->type == D3DResourceType::VOLTEX)
      slises = base->depth;
    else if (base->type == D3DResourceType::CUBETEX)
      slises = 6;
    else if (base->type == D3DResourceType::CUBEARRTEX)
      slises = 6 * base->depth;

    if (base->metal_format != base->metal_rt_format)
    {
      G_ASSERT(base->memoryless == false);
      rt_texture = [texture newTextureViewWithPixelFormat : base->metal_rt_format];
    }
    else
    {
      if (base->memoryless)
      {
        pTexDesc.textureType = base->samples > 1 ? MTLTextureType2DMultisample : MTLTextureType2D;
        pTexDesc.storageMode = MTLStorageModeMemoryless;
        pTexDesc.resourceOptions = MTLResourceStorageModeMemoryless;
        pTexDesc.sampleCount = base->samples; // base->samples;

        rt_texture = [render.device newTextureWithDescriptor : pTexDesc];
        if (rt_texture == nil)
          DAG_FATAL("Failed to allocate memoryless texture %s", base->getName());
      }
      else
        rt_texture = texture;
    }

    if (no_srgb)
      texture_no_srgb = [texture newTextureViewWithPixelFormat : format2Metal(base->base_format)];
    else
      texture_no_srgb = [texture retain];

    if (as_uint)
      texture_as_uint = [texture newTextureViewWithPixelFormat : MTLPixelFormatR32Uint];

    sub_texture = texture;
    sub_texture_no_srgb = texture_no_srgb;

    if (base->metal_format == MTLPixelFormatDepth32Float_Stencil8 && !base->memoryless)
    {
      G_ASSERT(base->type != D3DResourceType::ARRTEX && base->type != D3DResourceType::CUBETEX && base->type != D3DResourceType::CUBEARRTEX);
      stencil_read_texture = [texture newTextureViewWithPixelFormat : MTLPixelFormatX32_Stencil8
                                                        textureType : base->metal_type
                                                             levels : NSMakeRange(0, base->mipLevels)
                                                             slices : NSMakeRange(0, 1)];
    }

    for (int level = 0; level < base->mipLevels; ++level)
    {
      int row_pitch = 0, slice_pitch = 0;
      getStride(base->base_format, base->width, base->height, level, row_pitch, slice_pitch);
      texture_size += slice_pitch * slises;
    }

    SubMip sub_mip;

    sub_mip.minLevel = 0;
    sub_mip.maxLevel = base->mipLevels;
    sub_mip.tex = texture;

    sub_mip_textures.push_back(sub_mip);

    applyName(base->getName());

    add_texture_to_list(this);
  }

  Texture::ApiTexture::ApiTexture(Texture *base, id<MTLTexture> tex, const char* name)
    : base(base)
  {
    texture = tex;
    rt_texture = tex;
    sub_texture = texture;
    sub_texture_no_srgb = texture;

    SubMip sub_mip;

    sub_mip.minLevel = 0;
    sub_mip.maxLevel = base->mipLevels;
    sub_mip.tex = texture;

    sub_mip_textures.push_back(sub_mip);

    if (name)
      setTexName(texture, name);
  }

  void Texture::ApiTexture::release(bool immediate)
  {
    if (immediate)
    {
      if (rt_texture != texture)
        [rt_texture release];
      for (size_t i = 1; i < sub_mip_textures.size(); i++)
      {
        id<MTLTexture>& tex = sub_mip_textures[i].tex;
        [tex release];
      }
      sub_mip_textures.clear();

      if (texture)
        [texture release];
      if (stencil_read_texture)
        [stencil_read_texture release];
      if (texture_as_uint)
        [texture_as_uint release];
      if (texture_no_srgb)
        [texture_no_srgb release];
    }
    else
    {
      if (rt_texture != texture)
        render.queueResourceForDeletion(rt_texture);
      if (texture_as_uint)
        render.queueResourceForDeletion(texture_as_uint);
      if (stencil_read_texture)
        render.queueResourceForDeletion(stencil_read_texture);
      if (texture_no_srgb)
        render.queueResourceForDeletion(texture_no_srgb);
      for (size_t i = 1; i < sub_mip_textures.size(); i++)
      {
        id<MTLTexture>& tex = sub_mip_textures[i].tex;
        render.queueResourceForDeletion(tex);
      }
      sub_mip_textures.clear();

      render.queueResourceForDeletion(texture);
    }

    if (base && !base->waiting2delete && !base->memoryless)
      TEXQL_ON_RELEASE(base);

    g_textures.lock();
    if (g_textures.isIndexValid(tid))
      g_textures.safeReleaseEntry(tid);
    g_textures.unlock();
    tid = drv3d_generic::BAD_HANDLE;

    delete this;
  }

  void Texture::ApiTexture::applyName(const char *name)
  {
#if DAGOR_DBGLEVEL > 0
    if (base == nullptr || name == nullptr)
      return;

    G_ASSERT(texture);
    setTexName(texture, name);

    if (rt_texture != texture)
      setTexName(rt_texture, name);
    if (texture_as_uint)
      setTexName(texture_as_uint, name);
    if (stencil_read_texture)
      setTexName(stencil_read_texture, name);
    if (texture_no_srgb != texture)
      setTexName(texture_no_srgb, name);
    for (size_t i = 1; i < sub_mip_textures.size(); i++)
      setTexName(sub_mip_textures[i].tex, name);
#endif
  }

  void Texture::setApiName(const char *name) const
  {
    if (apiTex)
    {
      String tname;
      if (heap_offset != ~0u)
        tname.printf(0, "o %llu - %s", heap_offset, name);
      else
        tname.printf(0, "%s", name);
      apiTex->applyName(tname);
    }
  }

  Texture::Texture(int w, int h, int l, int d, D3DResourceType type, int flg, int fmt, const char* name, bool is_memoryless, bool alloc)
    : type(type)
    , cflg(flg)
    , memoryless(is_memoryless)
  {
    setName(name);

#if _TARGET_IOS | _TARGET_TVOS
    switch(fmt)
    {
      case TEXFMT_DXT1:
      case TEXFMT_DXT3:
      case TEXFMT_ATI1N:
      case TEXFMT_DXT5:
      case TEXFMT_ATI2N:
      case TEXFMT_BC6H:
      case TEXFMT_BC7:
            D3D_CONTRACT_ERROR("unsupported texture format %d is used for texture %s", fmt, getName());
            fmt = TEXFMT_ASTC4;
            break;
    }
#endif

    D3D_CONTRACT_ASSERTF(w > 0 && h > 0, "Weird texture %s dimensions %ix%i", getName(), w, h);
    width = w;
    height = h;

    if (cflg & TEXCF_SAMPLECOUNT_MASK)
    {
      memoryless = true;
      samples = get_sample_count(cflg);
    }
    if (cflg & TEXCF_TRANSIENT)
      memoryless = true;

    if (l < 1)
    {
      mipLevels = getMaxLevels(width, height);
    }
    else
    {
      mipLevels = l;

      if (l > getMaxLevels(width, height))
      {
        mipLevels = getMaxLevels(width, height);
      }
    }

    depth = d;

    start_level = 0;

    cflg = flg | (fmt & TEXFMT_MASK);
    if (cflg & (TEXCF_RTARGET|TEXCF_UNORDERED))
      cflg &= ~TEXCF_READABLE;

    if (depth == 0)
    {
      depth = 1;
    }

    base_format = fmt;

    if (cflg & TEXCF_SRGBREAD)
    {
      metal_format = format2MetalsRGB(fmt);
    }
    else
    {
      metal_format = format2Metal(fmt);
    }

    if (cflg & TEXCF_RTARGET)
    {
      metal_rt_format = cflg & TEXCF_SRGBWRITE ? format2MetalsRGB(fmt) : format2Metal(fmt);
    }
    else
    {
      metal_rt_format = metal_format;
    }

    lockFlags = 0;

    metal_type = MTLTextureType2D;

    if (type == D3DResourceType::CUBETEX)
    {
      D3D_CONTRACT_ASSERT(samples == 1);
      metal_type = MTLTextureTypeCube;
    }
    else
    if (type == D3DResourceType::VOLTEX)
    {
      D3D_CONTRACT_ASSERT(samples == 1);
      metal_type = MTLTextureType3D;
    }
    else
    if (type == D3DResourceType::ARRTEX)
    {
      D3D_CONTRACT_ASSERT(samples == 1);
      metal_type = MTLTextureType2DArray;
    }
#if !_TARGET_TVOS
    else
    if (type == D3DResourceType::CUBEARRTEX)
    {
      D3D_CONTRACT_ASSERT(samples == 1);
      metal_type = MTLTextureTypeCubeArray;
    }
#endif

    use_dxt = 0;

    if (base_format == TEXFMT_DXT1 || base_format == TEXFMT_DXT3 || base_format == TEXFMT_DXT5 ||
      base_format == TEXFMT_ATI1N || base_format == TEXFMT_ATI2N ||
      base_format == TEXFMT_BC6H || base_format == TEXFMT_BC7 ||
      base_format == TEXFMT_ETC2_RGBA || base_format == TEXFMT_ETC2_RG || base_format == TEXFMT_ASTC4)
    {
      use_dxt = 1;
    }

    use_upload_buffer = true;

#if _TARGET_PC_MACOSX
    // no memoryless on mac cause we haven't implemented imageblocks yet
    //if (!render.device.hasUnifiedMemory || render.device.lowPower)
      memoryless = false;
#endif

    waiting2delete = false;

    if (alloc)
      allocate();

    D3D_CONTRACT_ASSERT(!(flg & TEXCF_CLEAR_ON_CREATE) || alloc);
    bool clearOnCreate = flg & TEXCF_CLEAR_ON_CREATE ||
                         (alloc && render.forceClearOnCreate && (fmt < TEXFMT_FIRST_DEPTH || fmt > TEXFMT_LAST_DEPTH));
    if (clearOnCreate)
    {
      D3D_CONTRACT_ASSERT(fmt < TEXFMT_FIRST_DEPTH || fmt > TEXFMT_LAST_DEPTH);
      render.clearTexture(this);
    }
  }

  Texture::Texture(id<MTLTexture> tex, int fmt, const char* name)
  {
    waiting2delete = false;

    type = D3DResourceType::TEX;
    cflg = TEXCF_RTARGET;

    base_format = fmt;
    metal_format = Texture::format2Metal(fmt);
    metal_rt_format = Texture::format2Metal(fmt);

    setName(name);

    apiTex = new ApiTexture(this, tex, name);
  }

  Texture::Texture(id<MTLTexture> tex, MTLTextureDescriptor *desc, uint32_t flg, const char *name)
  {
    setName(name);

    cflg = flg;
    metal_type = desc.textureType;
    if (metal_type == MTLTextureType2D)
      type = D3DResourceType::TEX;
    else if (metal_type == MTLTextureType3D)
      type = D3DResourceType::VOLTEX;
    else if (metal_type == MTLTextureTypeCube)
      type = D3DResourceType::CUBETEX;
    else if (metal_type == MTLTextureTypeCubeArray)
      type = D3DResourceType::CUBEARRTEX;
    else if (metal_type == MTLTextureType2DArray)
      type = D3DResourceType::ARRTEX;

    width = desc.width;
    height = desc.height;
    depth = desc.depth;
    samples = desc.sampleCount;
    mipLevels = desc.mipmapLevelCount;
    start_level = 0;

    base_format = flg & TEXFMT_MASK;


    metal_format = cflg & TEXCF_SRGBREAD ? format2MetalsRGB(base_format) : format2Metal(base_format);
    metal_rt_format = cflg & TEXCF_RTARGET ? (cflg & TEXCF_SRGBWRITE ? format2MetalsRGB(base_format) : format2Metal(base_format)) : metal_format;

    lockFlags = 0;

    use_dxt = 0;

    if (base_format == TEXFMT_DXT1 || base_format == TEXFMT_DXT3 || base_format == TEXFMT_DXT5 ||
      base_format == TEXFMT_ATI1N || base_format == TEXFMT_ATI2N ||
      base_format == TEXFMT_BC6H || base_format == TEXFMT_BC7 ||
      base_format == TEXFMT_ETC2_RGBA || base_format == TEXFMT_ETC2_RG)
    {
      use_dxt = 1;
    }

    use_upload_buffer = true;

    memoryless = false;
    waiting2delete = false;

    apiTex = new ApiTexture(this, tex, name);
  }

  Texture::~Texture()
  {
  }

  void Texture::allocate()
  {
    D3D_CONTRACT_ASSERT(apiTex == nullptr);
    apiTex = new ApiTexture(this);
    if (!memoryless)
      TEXQL_ON_ALLOC(this);
    if (start_level)
      setMinMipLevel(start_level);
  }

  uint32_t Texture::getSize() const
  {
    return apiTex ? apiTex->texture_size : 0;
  }

  void Texture::releaseTex(bool recreate_after)
  {
    D3D_CONTRACT_ASSERT(lockFlags == 0); //locked?
    if (isStub())
      apiTex = NULL;
    if (apiTex != NULL)
    {
      apiTex->release(false);
      apiTex = NULL;
    }
  }

  void Texture::discardTex()
  {
    if (stubTexIdx >= 0)
    {
      RCAutoLock texLock;

      releaseTex(true);
      apiTex = getStubTex()->apiTex;
    }
  }

  bool Texture::updateTexResFormat(unsigned d3d_format)
  {
    if (!isStub())
      return cflg == implant_d3dformat(cflg, d3d_format);
    cflg = implant_d3dformat(cflg, d3d_format);
    base_format = cflg & TEXFMT_MASK;
    metal_format = (cflg & TEXCF_SRGBREAD) ? Texture::format2MetalsRGB(base_format) : Texture::format2Metal(base_format);
    if (cflg & TEXCF_RTARGET)
      metal_rt_format = (cflg & TEXCF_SRGBWRITE) ? Texture::format2MetalsRGB(base_format) : Texture::format2Metal(base_format);
    else
      metal_rt_format = metal_format;
    return true;
  }

  BaseTexture *Texture::makeTmpTexResCopy(int w, int h, int d, int l)
  {
    if (type != D3DResourceType::ARRTEX && type != D3DResourceType::VOLTEX && type != D3DResourceType::CUBEARRTEX)
      d = 1;
    Texture *clonedTex =
      new Texture(w, h, l, d, type, cflg, (cflg & TEXFMT_MASK), String::mk_str_cat("tmp:", getTexName()), memoryless, false);
    clonedTex->tidXored = tidXored, clonedTex->stubTexIdx = stubTexIdx;
    if (!clonedTex->allocateTex())
      del_d3dres(clonedTex);
    return clonedTex;
  }

  void Texture::replaceTexResObject(BaseTexture *&other_tex)
  {
    if (this == other_tex)
      return;

    render.checkRenderAcquired();

    Texture* other = getbasetex(other_tex);
    G_ASSERT_RETURN(other, );

    G_ASSERT(lockFlags == 0 && other->lockFlags == 0);

    std::swap(apiTex, other->apiTex);
    std::swap(width, other->width);
    std::swap(height, other->height);
    std::swap(mipLevels, other->mipLevels);
    std::swap(depth, other->depth);
    if (apiTex && !tql::isTexStub(apiTex->base))
    {
      apiTex->base = this;
      apiTex->applyName(getName());
    }
    if (other->apiTex && !tql::isTexStub(other->apiTex->base))
    {
      other->apiTex->base = other;
      other->apiTex->applyName(other->getName());
    }
    del_d3dres(other_tex);
    other_tex = nullptr;
  }

  int Texture::getWidth()
  {
    return width;
  }

  int Texture::getHeight()
  {
    return height;
  }

  int Texture::getinfo(TextureInfo &ti, int level) const
  {
    if (level >= mipLevels)
      level = mipLevels - 1;

    ti.w = max<uint32_t>(1u, width >> level);
    ti.h = max<uint32_t>(1u, height >> level);
    switch (type)
    {
      case D3DResourceType::CUBETEX:    ti.d = 1; ti.a = 6; break;
      case D3DResourceType::ARRTEX:     ti.d = 1; ti.a = depth; break;
      case D3DResourceType::CUBEARRTEX: ti.d = 1; ti.a = 6*depth; break;
      case D3DResourceType::VOLTEX:     ti.d = max<uint32_t>(1u, depth >> level); ti.a = 1; break;
      default:               ti.d = 1; ti.a = 1; break;
    }

    ti.mipLevels = mipLevels;
    ti.type = type;
    ti.cflg = cflg;
    return 1;
  }

  int Texture::level_count() const
  {
    return mipLevels;
  }

  int Texture::texmiplevel(int set_minlevel, int set_maxlevel)
  {
    set_minlevel = (set_minlevel >= 0) ? set_minlevel : 0;
    set_maxlevel = (set_maxlevel >= 0) ? set_maxlevel + 1 : mipLevels;

    G_ASSERT(apiTex);
    D3D_CONTRACT_ASSERT(isStub() || apiTex->base == this);
    if (!isStub())
    {
      D3D_CONTRACT_ASSERT(mipLevels >= 1);
      set_minlevel = min(set_minlevel, (int)mipLevels-1);
      set_maxlevel = min(set_maxlevel, (int)mipLevels);
      apiTex->sub_texture = apiTex->allocateOrCreateSubmip(set_minlevel, set_maxlevel, false, 0);
    }
    return 1;
  }

  id<MTLTexture> Texture::ApiTexture::allocateOrCreateSubmip(int set_minlevel, int set_maxlevel, bool is_uav, int slice)
  {
    G_ASSERT(base);
    for (auto & sub_mip : sub_mip_textures)
    {
      if (sub_mip.minLevel == set_minlevel &&
          sub_mip.maxLevel == set_maxlevel &&
          sub_mip.is_uav == is_uav &&
          sub_mip.slice == slice)
      {
        return sub_mip.tex;
      }
    }

    SubMip sub_mip;

    sub_mip.minLevel = set_minlevel;
    sub_mip.maxLevel = set_maxlevel;
    sub_mip.is_uav = is_uav;
    sub_mip.slice = slice;

    int slises = 1;

    if (base->type == D3DResourceType::ARRTEX)
    {
      slises = base->depth;
    }

    if (base->type == D3DResourceType::CUBETEX)
    {
      slises = 6;
    }

    if (base->type == D3DResourceType::CUBEARRTEX)
    {
      slises = 6 * base->depth;
    }

    D3D_CONTRACT_ASSERT(slice < slises);
    sub_mip.tex = [texture newTextureViewWithPixelFormat : is_uav ? format2Metal(base->base_format) : base->metal_format
                                             textureType : base->metal_type
                                                  levels : NSMakeRange(set_minlevel, set_maxlevel - set_minlevel)
                                                  slices : NSMakeRange(slice, slises - slice)];

    if (base->getName()[0])
    {
      setTexName(sub_mip.tex, String(0, "%s (%d %d) (%d %d)", base->getName(), set_minlevel, set_maxlevel, slice, slises));
    }

    sub_mip_textures.push_back(sub_mip);

    return sub_mip.tex;
  }

  void Texture::apply(id<MTLTexture>& out_tex, bool is_read_stencil, int mip_level, int slice, bool is_uav, bool as_uint)
  {
    G_ASSERT(apiTex);
    D3D_CONTRACT_ASSERT(isStub() || apiTex->base == this || apiTex->base == nullptr);
    G_ASSERT(this != render.backbuffer || apiTex->sub_texture);
    if (as_uint)
    {
      D3D_CONTRACT_ASSERT(apiTex->texture_as_uint);
      out_tex = apiTex->texture_as_uint;
    }
    else if (is_read_stencil && apiTex->stencil_read_texture)
      out_tex = apiTex->stencil_read_texture;
    else
      out_tex = is_uav ? apiTex->sub_texture_no_srgb : apiTex->sub_texture;
    if (slice)
    {
      D3D_CONTRACT_ASSERT(mip_level == 0);
      D3D_CONTRACT_ASSERT(!as_uint);
      D3D_CONTRACT_ASSERT(!(is_read_stencil && apiTex->stencil_read_texture));
      out_tex = apiTex->allocateOrCreateSubmip(0, mipLevels, is_uav, slice);
    }
    else if (mip_level)
    {
      D3D_CONTRACT_ASSERT(slice == 0);
      D3D_CONTRACT_ASSERT(!as_uint);
      D3D_CONTRACT_ASSERT(!(is_read_stencil && apiTex->stencil_read_texture));
      out_tex = apiTex->allocateOrCreateSubmip(mip_level, mip_level+1, is_uav, 0);
    }
  }

  int Texture::lockimg(void **data, int &row_pitch, int level, unsigned flags)
  {
    int slice_pitch = 0;
    void* d = lock(row_pitch, slice_pitch, level, 0, flags, data == nullptr);

    if (data)
    {
      *data = d;
    }

    return 1;
  }

  int Texture::lockimg(void **data, int &row_pitch, int face, int level, unsigned flags)
  {
    int slice_pitch = 0;
    void* d = lock(row_pitch, slice_pitch, level, face, flags, !data);

    if (data)
    {
      *data = d;
    }

    return 1;
  }

  int Texture::unlockimg()
  {
    unlock();
    return 1;
  }

  int Texture::lockbox(void **data, int &row_pitch, int &slice_pitch, int level, unsigned flags)
  {
    void* d = lock(row_pitch, slice_pitch, level, 0, flags, !data);

    if (data)
    {
      *data = d;
    }

    return 1;
  }

  int Texture::unlockbox()
  {
    unlock();
    return 1;
  }

  int Texture::generateMips()
  {
    if (mipLevels == 1)
    {
      return 1;
    }

    render.generateMips(this);
    return 1;
  }

  int Texture::calcBaseLevel(int w, int h)
  {
    int base_level = 0;

    int wdt = width;
    int hgt = height;

    while (wdt != w && hgt != h)
    {
      wdt = wdt >> 1;
      hgt = hgt >> 1;
      base_level++;
    }

    return base_level;
  }

  void Texture::setBaseLevel(int base_level)
  {
    if (start_level != base_level)
    {
      start_level = base_level;
      setMinMipLevel(start_level);
    }
  }

  void Texture::setMinMipLevel(int min_mip_level)
  {
    if (!isStub())
      texmiplevel(min_mip_level, mipLevels);
  }

  void* Texture::lock(int &row_pitch, int &slice_pitch, int level, int face, unsigned flags, bool readback)
  {
    D3D_CONTRACT_ASSERT(lockFlags == 0);
    D3D_CONTRACT_ASSERTF(waiting2delete == false, "Trying to lock texture that is destroyed %s", getName());

    if (level >= mipLevels)
    {
      level = mipLevels - 1;
    }

    lockFlags = flags;
    if (readback)
      lockFlags |= TEX_COPIED;

    getStride(base_format, width, height, level, row_pitch, slice_pitch);

    SysImage* sys_image = NULL;

    for (int i = 0; i < sys_images.size(); i++)
    {
      if (sys_images[i].lock_level == level &&
          sys_images[i].lock_face == face)
      {
        sys_image = &sys_images[i];
      }
    }

    bool force_readback = last_readback_submit == ~0ull;
    if (!sys_image)
    {
      sys_images.push_back(SysImage());

      sys_image = &sys_images.back();

      sys_image->lock_level = level;
      sys_image->lock_face = face;
      sys_image->widBytes = row_pitch;
      sys_image->nBytes = slice_pitch;

      int d = type == D3DResourceType::VOLTEX ? max(depth >> sys_image->lock_level, 1) : 1;

      @autoreleasepool
      {
        String name;
        name.printf(0, "upload for %s %llu", getName(), render.frame);
        sys_image->buffer = render.createBuffer(slice_pitch * d, MTLResourceStorageModeShared, name);
        sys_image->image = (char*)sys_image->buffer.contents;
      }

      force_readback = true;
    }

    if ((lockFlags & TEXLOCK_READ))
    {
      D3D_CONTRACT_ASSERT(!(lockFlags & TEXLOCK_DONOTUPDATE));
      if (readback || force_readback)
      {
        int mins = use_dxt ? 4 : 1;
        int w = max(width >> (sys_image->lock_level), mins);
        int h = max(height >> (sys_image->lock_level), mins);
        int d = max(depth >> sys_image->lock_level, 1);

        G_ASSERT(apiTex);
        D3D_CONTRACT_ASSERT(sys_image->buffer);
        render.acquireOwnership();
        render.readbackTexture(this, sys_image->lock_level, sys_image->lock_face, w, h, d,
          sys_image->buffer, 0, sys_image->widBytes, sys_image->nBytes);
        last_readback_submit = render.submits_scheduled;
        render.releaseOwnership();
      }
      if (!readback && (render.submits_completed < last_readback_submit))
      {
#if DAGOR_DBGLEVEL > 0
        if (render.report_stalls)
          D3D_CONTRACT_ERROR("[METAL_TEXTURE] flushing to readback texture %s, frame %llu (%llu)", getName(), render.frame, render.submits_completed);
#endif
        TIME_PROFILE(textureReadbackFlush);
        @autoreleasepool
        {
          render.acquireOwnership();
          render.flush(true);
          render.releaseOwnership();
        }
      }
    }

    return sys_image->image;
  }

  void Texture::unlock()
  {
    if (lockFlags == 0)
    {
      return;
    }

    if (lockFlags & TEXLOCK_DONOTUPDATE)
    {
      lockFlags = 0;
      return;
    }

    if (type == D3DResourceType::ARRTEX && isStub())
    {
      if (sys_images.size() != mipLevels*depth)
      {
        lockFlags = 0;
        return;
      }
      apiTex = nullptr;
      allocate();
    }

    bool keep_upload_buffer = (cflg & TEXCF_READABLE) || (lockFlags & TEXLOCK_READ);
    for (int i=0; i<sys_images.size();i++)
    {
      SysImage& sys_image = sys_images[i];

      if (lockFlags & TEXLOCK_WRITE)
      {
        int w = max(width >> (sys_image.lock_level), 1);
        int h = max(height >> (sys_image.lock_level), 1);
        int d = type == D3DResourceType::VOLTEX ? max(depth >> sys_image.lock_level, 1) : 1;

        G_ASSERT(apiTex);
        render.uploadTexture(this, apiTex->texture, sys_image.lock_level, sys_image.lock_face, 0, 0, 0, w, h, d,
                                sys_image.buffer, sys_image.widBytes, sys_image.nBytes);
      }

      if (!keep_upload_buffer)
      {
        if (sys_image.buffer)
        {
          render.queueResourceForDeletion(sys_image.buffer);
          sys_image.buffer = nil;
        }
        else if (sys_image.image)
          free(sys_image.image);
        sys_image.image = nullptr;
      }
      else if (!(lockFlags & TEX_COPIED))
        last_readback_submit = ~0ull;
    }

    if (!keep_upload_buffer)
    {
      sys_images.clear();
    }

    lockFlags = 0;
  }

  int Texture::update(BaseTexture* src)
  {
    render.acquireOwnership();
    render.copyTex((Texture*)src, this);
    render.releaseOwnership();
    return 1;
  }

  int Texture::updateSubRegion(BaseTexture* src,
                          int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
                          int dest_subres_idx, int dest_x, int dest_y, int dest_z)
  {
    D3D_CONTRACT_ASSERT(!isStub());

    if (!validate_update_sub_region_params(src, src_subres_idx, src_x, src_y, src_z, src_w, src_h, src_d,
           this, dest_subres_idx, dest_x, dest_y, dest_z))
      return 0;

    render.copyTexRegion((Texture*)src, src_subres_idx, src_x, src_y, src_z, src_w, src_h, src_d,
                         this, dest_subres_idx, dest_x, dest_y, dest_z);
    return 1;
  }

  void Texture::destroy()
  {
    waiting2delete = true;
    if (!memoryless)
      TEXQL_ON_RELEASE(this);
    render.deleteTexture(this);
  }

  void Texture::release()
  {
    D3D_CONTRACT_ASSERTF(waiting2delete, "Trying to release texture that is not destroyed %s", getName());
    D3D_CONTRACT_ASSERTF(lockFlags == 0, "Trying to release locked texture %s", getName());
    for (int i = 0; i<sys_images.size(); i++)
    {
      if (!sys_images[i].buffer)
        free(sys_images[i].image);
      else
        [sys_images[i].buffer release];
    }

    if (apiTex && !isStub())
      apiTex->release(true);
    apiTex = nullptr;

    delete this;
  }

  void Texture::cleanup()
  {
    Tab<Texture::ApiTexture*> textures;
    g_textures.lock();
    ITERATE_OVER_OBJECT_POOL(g_textures, textureNo)
      if (g_textures[textureNo].obj != NULL)
        textures.push_back(g_textures[textureNo].obj);
    ITERATE_OVER_OBJECT_POOL_RESTORE(g_textures);
    g_textures.unlock();
    for (int i = 0; i < textures.size(); ++i)
      textures[i]->release(true);
  }
}

#define FULL_TEX_STAT_LOG (DAGOR_DBGLEVEL > 0 || (_TARGET_64BIT && DAGOR_FORCE_LOGS))
struct MetalBtSortRec
{
  drv3d_metal::Texture *t;
  int szKb, lfu;

  MetalBtSortRec(drv3d_metal::Texture *_t): t(_t), szKb(tql::sizeInKb(_t->getSize())), lfu(tql::get_tex_lfu(_t->getTID())) {}
  drv3d_metal::Texture* operator->() { return t; }
};
static int sort_textures_by_size(const MetalBtSortRec *left, const MetalBtSortRec *right)
{
  if (int diff = left->szKb - right->szKb)
    return -diff;
  if (int diff = eastl::to_underlying(left->t->type) - eastl::to_underlying(right->t->type))
    return diff;
  if (int diff = strcmp(left->t->getName(), right->t->getName()))
    return diff;
  return right->lfu - left->lfu;
}

extern int get_ib_vb_mem_used(String *, int &);

static const char *tex_mipfilter_string(uint32_t mipFilter)
{
  static const char* filterMode[5] =
  {
    "default", "point", "linear", "unknown"
  };
  return filterMode[min(mipFilter, (uint32_t)4)];
}
static const char *tex_filter_string(uint32_t filter)
{
  static const char* filterMode[6] =
  {
    "default", "point", "smooth", "best", "none", "unknown"
  };
  return filterMode[min(filter, (uint32_t)5)];
}

static const char* format2String(uint32_t fmt)
  {
    switch (fmt)
    {
      case TEXFMT_A8R8G8B8:     return "A8R8G8B8";
      case TEXFMT_A2R10G10B10:  return "A2R10G10B10";
      case TEXFMT_A2B10G10R10:  return "A2B10G10R10";
      case TEXFMT_A16B16G16R16:   return "A16B16G16R16";
      case TEXFMT_A16B16G16R16F:  return "A16B16G16R16F";
      case TEXFMT_A32B32G32R32F:  return "A32B32G32R32F";
      case TEXFMT_G16R16:     return "G16R16";
      case TEXFMT_G16R16F:    return "G16R16F";
      case TEXFMT_G32R32F:    return "G32R32F";
      case TEXFMT_R16F:       return "R16F";
      case TEXFMT_R32F:       return "R32F";
      case TEXFMT_DXT1:       return "DXT1";
      case TEXFMT_DXT3:       return "DXT3";
      case TEXFMT_DXT5:       return "DXT5";
      case TEXFMT_ATI1N:      return "ATI1N";
      case TEXFMT_ATI2N:      return "ATI2N";
      case TEXFMT_BC6H:       return "BC6H";
      case TEXFMT_BC7:      return "BC7";
      case TEXFMT_ASTC4:          return "ASTC4";
      case TEXFMT_ASTC8:          return "ASTC8";
      case TEXFMT_ASTC12:         return "ASTC12";
      case TEXFMT_R32G32UI: return "R32G32UI";
      case TEXFMT_L16:      return "L16";
      case TEXFMT_A8:       return "A8";
      case TEXFMT_R8:       return "R8";
      case TEXFMT_A1R5G5B5:  return "A1R5G5B5";
      case TEXFMT_A4R4G4B4:     return "A4R4G4B4";
      case TEXFMT_R5G6B5:     return "R5G6B5";
      case TEXFMT_A16B16G16R16S:  return "A16B16G16R16S";
      case TEXFMT_A16B16G16R16UI: return "A16B16G16R16UI";
      case TEXFMT_A32B32G32R32UI: return "A32B32G32R32UI";
      case TEXFMT_R8G8B8A8:     return "R8G8B8A8";
      case TEXFMT_R16UI:      return "R16UI";
      case TEXFMT_R32UI:      return "R32UI";
      case TEXFMT_R32SI:      return "R32SI";
      case TEXFMT_R11G11B10F:   return "R11G11B10F";
      case TEXFMT_R8G8:       return "R8G8";
      case TEXFMT_R8G8S:      return "R8G8S";
      case TEXFMT_DEPTH24:    return "DEPTH24";
      case TEXFMT_DEPTH16:    return "DEPTH16";
      case TEXFMT_DEPTH32:    return "DEPTH32";
      case TEXFMT_DEPTH32_S8:   return "DEPTH32_S8";
    }

    return "Unknown";
  }

void d3d::get_texture_statistics(uint32_t *num_textures, uint64_t *total_mem, String *out_dump)
{
  g_textures.lock();

  int totalTextures = g_textures.totalUsed();
  if (num_textures)
    *num_textures = totalTextures;
  if (total_mem)
    *total_mem = 0;

  Tab<MetalBtSortRec> sortedTexturesList(tmpmem); sortedTexturesList.reserve(totalTextures);

  ITERATE_OVER_OBJECT_POOL(g_textures, textureNo)
    if (g_textures[textureNo].obj != NULL)
      sortedTexturesList.push_back(g_textures[textureNo].obj->base);
  ITERATE_OVER_OBJECT_POOL_RESTORE(g_textures);

  sort(sortedTexturesList, &sort_textures_by_size);

  uint64_t totalSize2d = 0;
  uint64_t totalSizeCube = 0;
  uint64_t totalSizeVol = 0;
  uint64_t totalSizeArr = 0;
  uint32_t numUnknownSize = 0;
  String tmpStor;
  for (unsigned int textureNo = 0; textureNo < sortedTexturesList.size(); textureNo++)
  {
    uint64_t sz = (uint64_t)sortedTexturesList[textureNo]->getSize();
    if (sz == 0)
    {
      numUnknownSize++;
    }
    else
    {
      if (sortedTexturesList[textureNo]->type == D3DResourceType::TEX)
        totalSize2d += sz;
      else if (sortedTexturesList[textureNo]->type == D3DResourceType::CUBETEX)
        totalSizeCube += sz;
      else if (sortedTexturesList[textureNo]->type == D3DResourceType::VOLTEX)
        totalSizeVol += sz;
      else if (sortedTexturesList[textureNo]->type == D3DResourceType::ARRTEX)
        totalSizeArr += sz;
    }
  }

  uint64_t texturesMem = totalSize2d + totalSizeCube + totalSizeVol + totalSizeArr;
  int vb_sysmem = 0;
  String vbibmem;
  uint64_t vb_ib_mem = get_ib_vb_mem_used(&vbibmem, vb_sysmem);

  if (total_mem)
    *total_mem = totalSize2d + totalSizeCube + totalSizeVol;

  if (out_dump)
  {
    *out_dump += String(200,
        "TOTAL: %dM\n"
        "%d textures: %dM (%llu), 2d: %dM (%llu), cube: %dM (%llu), vol: %dM (%llu), arr: %dM (%llu), %d stubs/unknown\n"
        "sysmem for drvReset: %dM (%dK)\n"
        "vb/ib mem: %dM\n",
        (vb_ib_mem >> 20) + texturesMem / 1024 / 1024,
        totalTextures,
        texturesMem / 1024 / 1024,
        texturesMem,
        totalSize2d / 1024 / 1024,
        totalSize2d,
        totalSizeCube / 1024 / 1024,
        totalSizeCube,
        totalSizeVol / 1024 / 1024,
        totalSizeVol,
        totalSizeArr / 1024 / 1024,
        totalSizeArr,
        numUnknownSize,
        vb_sysmem >> 20, vb_sysmem >> 10,
        vb_ib_mem >> 20);

    *out_dump += String(0, "\n");

    STAT_STR_BEGIN(add);
    for (unsigned int textureNo = 0; textureNo < sortedTexturesList.size(); textureNo++)
    {
      auto *tex = (drv3d_metal::Texture*)sortedTexturesList[textureNo].t;
      uint64_t sz = tql::sizeInKb(tex->getSize());
      int ql = tql::get_tex_cur_ql(tex->getTID());
      if (tex->type == D3DResourceType::TEX)
      {
        add.printf(0, "%5dK q%d  2d  %4dx%-4d, m%2d, %s - '%s'",
            sz, ql,
            tex->width,
            tex->height,
            tex->mipLevels,
            format2String(tex->base_format),
            tex->getName());
      }
      else if (tex->type == D3DResourceType::CUBETEX)
      {
        add.printf(0, "%5dK q%d cube %4d, m%2d, %s - '%s'",
            sz, ql,
            tex->width,
            tex->mipLevels,
            format2String(tex->base_format),
            tex->getName());
      }
      else
      {
        add.printf(0, "%5dK q%d  %s %4dx%-4dx%3d, m%2d, %s - '%s'",
            sz, ql, (tex->type == D3DResourceType::VOLTEX) ? "vol" : ((tex->type == D3DResourceType::ARRTEX) ? "arr" : " ? "),
            tex->width,
            tex->height,
            tex->depth,
            tex->mipLevels,
            format2String(tex->base_format),
            tex->getName());
      }

      add += num_textures ? "\n" : String(0, "%s\n", tql::get_tex_info(tex->getTID(), false, tmpStor)).str();

      STAT_STR_APPEND(add, *out_dump);
    }
    STAT_STR_END(*out_dump);
    *out_dump += vbibmem;

    // get_shaders_mem_used(vbibmem);
    // *out_dump += vbibmem;
    // get_states_mem_used(vbibmem);
    // *out_dump += vbibmem;
  }

  g_textures.unlock();
}
