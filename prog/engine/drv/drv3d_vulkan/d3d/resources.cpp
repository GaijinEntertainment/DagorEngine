// Copyright (C) Gaijin Games KFT.  All rights reserved.

// common resource-like objects managment implementation
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_res.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_platform.h>
#include <drv/3d/dag_tiledResource.h>
#include <image/dag_texPixel.h>
#include <validation/texture.h>
#include <validate_sbuf_flags.h>
#include <texture.h>
#include <buffer.h>
#include "globals.h"
#include "external_resource_pools.h"
#include "shader/pipeline_description.h"
#include "render_state_system.h"
#include "sampler_resource.h"
#include "sampler_cache.h"
#include "resourceActivationGeneric.h"
#include "resource_manager.h"
#include "device_context.h"

using namespace drv3d_vulkan;

namespace
{

bool invalidCreationArgs(uint32_t width, uint32_t height, uint32_t flags, const char *name)
{
  check_texture_creation_args(width, height, flags, name);
  if ((flags & (TEXCF_RTARGET | TEXCF_DYNAMIC)) == (TEXCF_RTARGET | TEXCF_DYNAMIC))
  {
    D3D_CONTRACT_ERROR("vulkan: can not create dynamic render target");
    return true;
  }
  D3D_CONTRACT_ASSERT(!((flags & TEXCF_LOADONCE) && (flags & (TEXCF_DYNAMIC | TEXCF_RTARGET))));

  return false;
}

} // namespace

// texture

TextureInterfaceBase *drv3d_vulkan::allocate_texture(uint16_t w, uint16_t h, uint16_t d, uint8_t levels, D3DResourceType type, int flg,
  const char *stat_name)
{
  return Globals::Res::tex.allocate({w, h, d, levels, type, flg}, stat_name);
}

void drv3d_vulkan::free_texture(TextureInterfaceBase *texture) { Globals::Res::tex.free(texture); }

Texture *d3d::create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name)
{
  if (invalidCreationArgs(w, h, flg, stat_name))
    return nullptr;

  if (img)
  {
    w = img->w;
    h = img->h;
  }

  const Driver3dDesc &dd = d3d::get_driver_desc();
  w = clamp<int>(w, dd.mintexw, dd.maxtexw);
  h = clamp<int>(h, dd.mintexh, dd.maxtexh);

  if (img)
  {
    levels = 1;

    if ((w != img->w) || (h != img->h))
    {
      D3D_CONTRACT_ERROR("create_tex: image size differs from texture size (%dx%d != %dx%d)", img->w, img->h, w, h);
      img = nullptr; // abort copying
    }

    if (FormatStore::fromCreateFlags(flg).getBytesPerPixelBlock() != 4)
      img = nullptr;
  }

  TextureInterfaceBase *tex = allocate_texture(w, h, 1, levels, D3DResourceType::TEX, flg, stat_name);

  if (tex->pars.flg & TEXCF_SYSTEXCOPY)
  {
    if (img)
      tex->storeSysCopy(img + 1);
    else if (tex->pars.flg & TEXCF_LOADONCE)
      tex->storeSysCopy(nullptr);
    else
      tex->pars.flg &= ~TEXCF_SYSTEXCOPY;
  }

  // only 1 mip
  BaseTex::ImageMem idata;
  if (img)
  {
    idata.ptr = img + 1;
    idata.rowPitch = w * 4;
  }

  if (!tex->allocateImage(img ? &idata : nullptr))
    return nullptr;

  return tex;
}

CubeTexture *d3d::create_cubetex(int size, int flg, int levels, const char *stat_name)
{
  if (invalidCreationArgs(size, size, flg, stat_name))
    return nullptr;

  const Driver3dDesc &dd = d3d::get_driver_desc();
  size = get_bigger_pow2(clamp<int>(size, dd.mincubesize, dd.maxcubesize));

  TextureInterfaceBase *tex = allocate_texture(size, size, 1, levels, D3DResourceType::CUBETEX, flg, stat_name);
  if (!tex->allocateTex())
    return nullptr;

  return tex;
}

VolTexture *d3d::create_voltex(int w, int h, int d, int flg, int levels, const char *stat_name)
{
  if (invalidCreationArgs(w, h, flg, stat_name))
    return nullptr;

  TextureInterfaceBase *tex = allocate_texture(w, h, d, levels, D3DResourceType::VOLTEX, flg, stat_name);
  if (!tex->allocateTex())
    return nullptr;

  if (tex->pars.flg & TEXCF_SYSTEXCOPY)
    tex->storeSysCopy(nullptr);

  return tex;
}

ArrayTexture *d3d::create_array_tex(int w, int h, int d, int flg, int levels, const char *stat_name)
{
  if (invalidCreationArgs(w, h, flg, stat_name))
    return nullptr;

  TextureInterfaceBase *tex = allocate_texture(w, h, d, levels, D3DResourceType::ARRTEX, flg, stat_name);
  if (!tex->allocateTex())
    return nullptr;

  return tex;
}

ArrayTexture *d3d::create_cube_array_tex(int side, int d, int flg, int levels, const char *stat_name)
{
  if (invalidCreationArgs(side, side, flg, stat_name))
    return nullptr;

  TextureInterfaceBase *tex = allocate_texture(side, side, d, levels, D3DResourceType::CUBEARRTEX, flg, stat_name);
  if (!tex->allocateTex())
    return nullptr;

  return tex;
}


#if _TARGET_PC_WIN
unsigned d3d::pcwin::get_texture_format(const BaseTexture *tex)
{
  auto bt = getbasetex(tex);
  if (!bt)
    return 0;
  return bt->getFormat();
}
const char *d3d::pcwin::get_texture_format_str(const BaseTexture *tex)
{
  auto bt = getbasetex(tex);
  if (!bt)
    return nullptr;
  return bt->getFormat().getNameString();
}
#endif

void d3d::get_texture_statistics(uint32_t *num_textures, uint64_t *total_mem, String *out_dump)
{
  if (num_textures)
    *num_textures = Globals::Res::tex.size();

  if (total_mem)
    Globals::Res::tex.iterateAllocated([total_mem](TextureInterfaceBase *tex) { *total_mem += tex->getSize(); });

  // TODO: make a proper one like in dx11/dx12/etc
  // but now at least make log on app.tex being called
  WinAutoLock lk(Globals::Mem::mutex);
  Globals::Mem::res.printStats(out_dump != nullptr, true);
}

// tiled (partial-resident) resources

void d3d::map_tile_to_resource(BaseTexture *tex, ResourceHeap *heap, const TileMapping *mapping, size_t mapping_count)
{
  D3D_CONTRACT_ASSERT_RETURN(tex, );
  D3D_CONTRACT_ASSERT_RETURN(mapping, );
  D3D_CONTRACT_ASSERT_RETURN(mapping_count, );
  D3D_CONTRACT_ASSERT_RETURN(tex->getType() != D3DResourceType::VOLTEX || d3d::get_driver_desc().caps.hasTiled3DResources, );
  D3D_CONTRACT_ASSERT_RETURN(tex->getType() != D3DResourceType::TEX || d3d::get_driver_desc().caps.hasTiled2DResources, );
  D3D_CONTRACT_ASSERT_RETURN(tex->getType() != D3DResourceType::CUBETEX || d3d::get_driver_desc().caps.hasTiled2DResources, );
  D3D_CONTRACT_ASSERT_RETURN(tex->getType() != D3DResourceType::ARRTEX || d3d::get_driver_desc().caps.hasTiled2DResources, );
  D3D_CONTRACT_ASSERT_RETURN(tex->getType() != D3DResourceType::CUBEARRTEX || d3d::get_driver_desc().caps.hasTiled2DResources, );
  BaseTex *texture = cast_to_texture_base(tex);
  D3D_CONTRACT_ASSERT_RETURN(texture->image, );
  Image *img = texture->image;
  MemoryHeapResource *dHeap = reinterpret_cast<MemoryHeapResource *>(heap);

  if (img->isSparseAspected())
  {
    D3D_CONTRACT_ERROR("vulkan: sparse resident image %p:%s is multi aspected, not supported!", img, img->getDebugName());
    return;
  }

  VkSparseImageMemoryRequirements sreqs = img->getSparseMemoryReqNonAspected();
  VkDeviceSize tileSize = img->getMemoryReq().requirements.alignment;
  VkExtent3D texelGranularity = sreqs.formatProperties.imageGranularity;

  ResourceMemory heapMem{};
  if (dHeap)
  {
    WinAutoLock lk(Globals::Mem::mutex);
    heapMem = dHeap->getMemory();
  }

  OSSpinlockScopedLock lockedFront(Globals::ctx.getFrontLock());

  intptr_t memBindOffset = Frontend::replay->sparseMemoryBinds.size();
  uint32_t memBinds = 0;

  intptr_t imgMemBindOffset = Frontend::replay->sparseImageMemoryBinds.size();
  uint32_t imgMemBinds = 0;

  for (const TileMapping &i : make_span(mapping, mapping_count))
  {
    G_ASSERTF(i.heapTileSpan, "vulkan: missing tiles to bind for %p:%s", img, img->getDebugName());
    if (!i.heapTileSpan)
      continue;
    D3D_CONTRACT_ASSERTF(img->getMipLevels() == texture->pars.levels, "vulkan: texture and image have different level count %u vs %u",
      img->getMipLevels(), texture->pars.levels);
    uint32_t mip = i.texSubresource % img->getMipLevels();
    uint32_t layer = i.texSubresource / img->getMipLevels();
    D3D_CONTRACT_ASSERTF(layer < img->getArrayLayers(),
      "vulkan: trying to bind tiles onto non existing subresource %u - mip:%u layer:%u", i.texSubresource, mip, layer);
    VkExtent3D subresExt = img->getMipExtents(mip);

    // tail mip area
    if (mip >= sreqs.imageMipTailFirstLod)
    {
      bool singleMiptail = sreqs.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT;
      VkDeviceSize mipTailSize = sreqs.imageMipTailSize * (singleMiptail ? 1 : img->getArrayLayers());
      G_UNUSED(mipTailSize);

      D3D_CONTRACT_ASSERTF(layer == 0, "vulkan: mip tail need to be bound to whole resource, avoid using layer != 0");
      D3D_CONTRACT_ASSERTF(i.texX == 0 && i.texY == 0 && i.texZ == 0,
        "vulkan: tail mip areas must be bound without offsets got [%u %u %u]!", i.texX, i.texY, i.texZ);
      G_ASSERTF(i.heapTileSpan * tileSize == mipTailSize, "vulkan: invalid size for mip tail provided %u != %u",
        i.heapTileSpan * tileSize, sreqs.imageMipTailSize);

      VkSparseMemoryBind bind{};
      if (dHeap)
      {
        bind.memory = heapMem.deviceMemory();
        G_ASSERTF((tileSize * i.heapTileIndex + mipTailSize) <= heapMem.size,
          "vulkan: out of heap bounds at mip tail tile mapping for %p:%s base tile %u, span size %u (in tiles)", img,
          img->getDebugName(), i.heapTileIndex, i.heapTileSpan);
        bind.memoryOffset = heapMem.offset + tileSize * i.heapTileIndex;
      }

      bind.resourceOffset = sreqs.imageMipTailOffset;
      bind.size = sreqs.imageMipTailSize;

      if (sreqs.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT)
      {
        Frontend::replay->sparseMemoryBinds.push_back(bind);
        ++memBinds;
      }
      else
      {
        for (int j = 0; j < img->getArrayLayers(); ++j)
        {
          Frontend::replay->sparseMemoryBinds.push_back(bind);
          ++memBinds;
          bind.resourceOffset += sreqs.imageMipTailStride;
          bind.memoryOffset += sreqs.imageMipTailSize;
        }
      }
    }
    else
    {
      VkSparseImageMemoryBind bind{};
      if (dHeap)
        bind.memory = heapMem.deviceMemory();
      bind.subresource.arrayLayer = layer;
      bind.subresource.mipLevel = mip;
      bind.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      bind.extent = texelGranularity;
      bind.offset.x = texelGranularity.width * i.texX;
      bind.offset.y = texelGranularity.height * i.texY;
      bind.offset.z = texelGranularity.depth * i.texZ;

      if (i.texX == 0 && i.texY == 0 && i.texZ == 0)
      {
        size_t totalTiles =
          max<size_t>(1, (subresExt.width / texelGranularity.width) + ((subresExt.width % texelGranularity.width) > 0 ? 1 : 0));
        totalTiles *=
          max<size_t>(1, (subresExt.height / texelGranularity.height) + ((subresExt.height % texelGranularity.height) > 0 ? 1 : 0));
        totalTiles *=
          max<size_t>(1, (subresExt.depth / texelGranularity.depth) + ((subresExt.depth % texelGranularity.depth) > 0 ? 1 : 0));
        if (totalTiles == i.heapTileSpan)
        {
          bind.extent = subresExt;
          VkDeviceSize heapMemOffset = tileSize * i.heapTileIndex;
          G_ASSERTF((heapMemOffset + tileSize * i.heapTileSpan) <= heapMem.size || !dHeap,
            "vulkan: out of heap bounds at tile mapping for %p:%s base tile %u, span area %u", img, img->getDebugName(),
            i.heapTileIndex, i.heapTileSpan);
          bind.memoryOffset = heapMem.offset + heapMemOffset;

          Frontend::replay->sparseImageMemoryBinds.push_back(bind);
          ++imgMemBinds;
          continue;
        }
      }

      for (int j = 0; j < i.heapTileSpan; ++j)
      {
        VkDeviceSize heapMemOffset = tileSize * (i.heapTileIndex + j);
        G_ASSERTF((heapMemOffset + tileSize) <= heapMem.size || !dHeap,
          "vulkan: out of heap bounds at tile mapping for %p:%s base tile %u, span index %u", img, img->getDebugName(),
          i.heapTileIndex, j);
        bind.memoryOffset = heapMem.offset + heapMemOffset;

        Frontend::replay->sparseImageMemoryBinds.push_back(bind);
        ++imgMemBinds;

        bind.offset.x += texelGranularity.width;
        if (bind.offset.x >= subresExt.width)
        {
          bind.offset.x = 0;
          bind.offset.y += texelGranularity.height;
          if (bind.offset.y >= subresExt.height)
          {
            bind.offset.y = 0;
            bind.offset.z += texelGranularity.depth;
          }
        }
      }
    }
  }

  // patch later at processing time
  if (memBinds)
    Frontend::replay->sparseImageOpaqueBinds.push_back({img->getHandle(), memBinds, (VkSparseMemoryBind *)memBindOffset});
  if (imgMemBinds)
    Frontend::replay->sparseImageBinds.push_back({img->getHandle(), imgMemBinds, (VkSparseImageMemoryBind *)imgMemBindOffset});
}

TextureTilingInfo d3d::get_texture_tiling_info(BaseTexture *tex, size_t subresource)
{
  D3D_CONTRACT_ASSERT_RETURN(tex, TextureTilingInfo{});
  BaseTex *texture = cast_to_texture_base(tex);
  G_ASSERT_RETURN(texture->image, TextureTilingInfo{});
  Image *img = texture->image;

  if (img->isSparseAspected())
  {
    D3D_CONTRACT_ERROR("vulkan: sparse resident image %p:%s is multi aspected, not supported!", img, img->getDebugName());
    return TextureTilingInfo{};
  }

  VkSparseImageMemoryRequirements sreqs = img->getSparseMemoryReqNonAspected();
  // VkMemoryRequirements::alignment field specifies the binding granularity in bytes for the image
  // min granularity == tile size as it is smallest portion of memory that can be bound
  VkDeviceSize tileSize = img->getMemoryReq().requirements.alignment;
  // regardless of this flag, externally we treat mip tails as one range
  bool singleMiptail = sreqs.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT;
  // bool alignedMiptail = sreqs.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT;

  // while spec says imageGranularity is in texels or compressed texel blocks
  // tyring to convert from blocks to texels leads to errors, so do nothing about it
  VkExtent3D texelGranularity = sreqs.formatProperties.imageGranularity;

  auto tileOffsetForNonTailMip = [img, texelGranularity, &sreqs](uint32_t mip) {
    uint32_t ret = 0;
    for (uint32_t i = 0; i < mip; ++i)
    {
      if (i >= sreqs.imageMipTailFirstLod)
        break;
      VkExtent3D mipExt = img->getMipExtents(i);
      uint32_t acm = (mipExt.width / texelGranularity.width) + ((mipExt.width % texelGranularity.width) ? 1 : 0);
      acm *= (mipExt.height / texelGranularity.height) + ((mipExt.height % texelGranularity.height) ? 1 : 0);
      acm *= (mipExt.depth / texelGranularity.depth) + ((mipExt.depth % texelGranularity.depth) ? 1 : 0);
      ret += acm;
    }
    return ret;
  };

  VkDeviceSize mipChainStrideInTiles = tileOffsetForNonTailMip(img->getMipLevels());
  VkDeviceSize mipTailSize = sreqs.imageMipTailSize * (singleMiptail ? 1 : img->getArrayLayers());
  VkDeviceSize reqMemorySize = mipChainStrideInTiles * img->getArrayLayers() * tileSize + mipTailSize;

  G_ASSERTF((sreqs.imageMipTailSize % tileSize) == 0,
    "vulkan: required mip tail memory size is not aligned to tile size %llu %% %llu = %u", sreqs.imageMipTailSize, tileSize,
    sreqs.imageMipTailSize % tileSize);
  G_ASSERTF((reqMemorySize % tileSize) == 0, "vulkan: required memory size is not aligned to tile size %u %% %u = %u", reqMemorySize,
    tileSize, reqMemorySize % tileSize);
  G_ASSERTF(sreqs.imageMipTailOffset % tileSize == 0, "vulkan: mip tail zero offset is not aligned to tile size %llu %% %llu = %u",
    sreqs.imageMipTailOffset, tileSize, sreqs.imageMipTailOffset % tileSize);

  // per layer mip tail may live in different places, but single mip tail can be checked for correctness
  G_ASSERTF((mipChainStrideInTiles * img->getArrayLayers() * tileSize == sreqs.imageMipTailOffset) || !singleMiptail,
    "vulkan: mip tail not after mip tiles %u %u %u", mipChainStrideInTiles, tileSize, sreqs.imageMipTailOffset);

  TextureTilingInfo ret{};
  ret.totalNumberOfTiles = reqMemorySize / tileSize;
  ret.numUnpackedMips = sreqs.imageMipTailFirstLod;
  ret.numPackedMips = img->getMipLevels() - ret.numUnpackedMips;
  ret.numTilesNeededForPackedMips = sreqs.imageMipTailSize * (singleMiptail ? 1 : img->getArrayLayers()) / tileSize;
  ret.firstPackedTileIndex = mipChainStrideInTiles * img->getArrayLayers();
  ret.tileWidthInPixels = texelGranularity.width;
  ret.tileHeightInPixels = texelGranularity.height;
  ret.tileDepthInPixels = texelGranularity.depth;
  ret.tileMemorySize = tileSize;

  uint32_t mip = subresource % img->getMipLevels();
  uint32_t layer = subresource / img->getMipLevels();
  VkExtent3D subresExt = img->getMipExtents(mip);

  ret.subresourceWidthInTiles =
    max<size_t>(1, (subresExt.width / texelGranularity.width) + ((subresExt.width % texelGranularity.width) > 0 ? 1 : 0));
  ret.subresourceHeightInTiles =
    max<size_t>(1, (subresExt.height / texelGranularity.height) + ((subresExt.height % texelGranularity.height) > 0 ? 1 : 0));
  ret.subresourceDepthInTiles =
    max<size_t>(1, (subresExt.depth / texelGranularity.depth) + ((subresExt.depth % texelGranularity.depth) > 0 ? 1 : 0));
  ret.subresourceStartTileIndex = mipChainStrideInTiles * layer + tileOffsetForNonTailMip(mip);

  return ret;
}

// buffer

GenericBufferInterface *drv3d_vulkan::allocate_buffer(uint32_t struct_size, uint32_t element_count, uint32_t flags, FormatStore format,
  bool managed, const char *stat_name)
{
  validate_sbuffer_flags(flags, stat_name);
  return Globals::Res::buf.allocate(struct_size, element_count, flags, format, managed, stat_name);
}

void drv3d_vulkan::free_buffer(GenericBufferInterface *buffer) { Globals::Res::buf.free(buffer); }

Vbuffer *d3d::create_vb(int size, int flg, const char *name)
{
  return allocate_buffer(1, size, flg | SBCF_BIND_VERTEX | SBCF_BIND_SHADER_RES, FormatStore(), true /*managed*/, name);
}

Ibuffer *d3d::create_ib(int size, int flg, const char *stat_name)
{
  return allocate_buffer(1, size, flg | SBCF_BIND_INDEX | SBCF_BIND_SHADER_RES, FormatStore(), true /*managed*/, stat_name);
}

Vbuffer *d3d::create_sbuffer(int struct_size, int elements, unsigned flags, unsigned format, const char *name)
{
  return allocate_buffer(struct_size, elements, flags, FormatStore::fromCreateFlags(format), true /*managed*/, name);
}

// render state

shaders::DriverRenderStateId d3d::create_render_state(const shaders::RenderState &state)
{
  return Globals::renderStates.registerState(Globals::ctx, state);
}

void d3d::clear_render_states()
{
  // do nothing
}

// VDECL

VDECL d3d::create_vdecl(VSDTYPE *vsd)
{
  drv3d_vulkan::InputLayout layout;
  layout.fromVdecl(vsd);
  return Globals::shaderProgramDatabase.registerInputLayout(Globals::ctx, layout).get();
}

void d3d::delete_vdecl(VDECL vdecl)
{
  (void)vdecl;
  // ignore delete request, we keep it as a 'optimization'
}

// Sampler

inline SamplerState translate_d3d_samplerinfo_to_vulkan_samplerstate(const d3d::SamplerInfo &samplerInfo)
{
  SamplerState result;
  result.setMip(translate_mip_filter_type_to_vulkan(int(samplerInfo.mip_map_mode)));
  result.setFilter(translate_filter_type_to_vulkan(int(samplerInfo.filter_mode)));
  result.setIsCompare(int(samplerInfo.filter_mode) == TEXFILTER_COMPARE);
  result.setW(translate_texture_address_mode_to_vulkan(int(samplerInfo.address_mode_w)));
  result.setV(translate_texture_address_mode_to_vulkan(int(samplerInfo.address_mode_v)));
  result.setU(translate_texture_address_mode_to_vulkan(int(samplerInfo.address_mode_u)));

  // TODO: no info about texture format (isSampledAsFloat)
  // use VkSamplerCustomBorderColorCreateInfoEXT with customBorderColorWithoutFormat feature
  result.setBorder(result.remap_border_color(samplerInfo.border_color, true));

  result.setBias(samplerInfo.mip_map_bias);
  result.setAniso(clamp<float>(samplerInfo.anisotropic_max, 1, 16));

  return result;
}

d3d::SamplerHandle d3d::request_sampler(const d3d::SamplerInfo &sampler_info)
{
  SamplerState state = translate_d3d_samplerinfo_to_vulkan_samplerstate(sampler_info);
  SamplerResource *ret = Globals::samplers.getResource(state);

  return d3d::SamplerHandle(reinterpret_cast<uint64_t>(ret));
}

// misc

void d3d::reserve_res_entries(bool strict_max, int max_tex, int max_vs, int max_ps, int max_vdecl, int max_vb, int max_ib,
  int max_stblk)
{
  Globals::Res::tex.reserve(max_tex);
  Globals::Res::buf.reserve(max_vb + max_ib);
  G_UNUSED(strict_max);
  G_UNUSED(max_tex);
  G_UNUSED(max_vs);
  G_UNUSED(max_ps);
  G_UNUSED(max_vdecl);
  G_UNUSED(max_stblk);
}

void d3d::get_max_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  max_tex = Globals::Res::tex.capacity();
  max_vb = max_ib = 0;
  Globals::Res::buf.iterateAllocated([&](auto buffer) {
    int flags = buffer->getFlags();
    if ((flags & SBCF_BIND_MASK) == SBCF_BIND_INDEX)
      ++max_ib;
    else
      ++max_vb;
  });

  auto total = max_vb + max_ib;
  max_vb *= Globals::Res::buf.capacity();
  max_ib *= Globals::Res::buf.capacity();
  max_vb /= total;
  max_ib /= total;
  G_UNUSED(max_vs);
  G_UNUSED(max_ps);
  G_UNUSED(max_vdecl);
  G_UNUSED(max_stblk);
}

void d3d::get_cur_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  max_tex = Globals::Res::tex.size();
  max_vb = max_ib = 0;
  Globals::Res::buf.iterateAllocated([&](auto buffer) {
    int flags = buffer->getFlags();
    if ((flags & SBCF_BIND_MASK) == SBCF_BIND_INDEX)
      ++max_ib;
    else
      ++max_vb;
  });

  G_UNUSED(max_vs);
  G_UNUSED(max_ps);
  G_UNUSED(max_vdecl);
  G_UNUSED(max_stblk);
}

IMPLEMENT_D3D_RESOURCE_ACTIVATION_API_USING_GENERIC()
