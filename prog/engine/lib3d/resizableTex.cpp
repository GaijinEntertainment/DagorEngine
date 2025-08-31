// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_info.h>
#include <3d/dag_resizableTex.h>
#include <math/dag_adjpow2.h>
#include <debug/dag_debug.h>
#include <EASTL/fixed_string.h>
#include <EASTL/algorithm.h>
#include <drv/3d/dag_texture.h>

namespace resptr_detail
{

using namespace resizeable_detail;

static key_t make_key(uint32_t width, uint32_t height, uint32_t d)
{
  const uint32_t sz = width * height * d;
  G_ASSERT(sz < (64 << 20ul) && d < 1024 && height < 16384 && width < 16384); // 64 mb is maximum size, 16384x16384x1024 is max dim
  return (uint64_t(sz) << 36ull) | uint64_t(uint64_t(d) << 28ull) | (uint64_t(height) << 14ull) | uint64_t(width);
}

void ResizableManagedTex::calcKey()
{
  currentKey = 0;
  lastMResId = mResId;
  if (mResource)
  {
    TextureInfo tex_info;
    mResource->getinfo(tex_info);
    currentKey = make_key(tex_info.w, tex_info.h, tex_info.d);
  }
}

static UniqueBaseTex try_get_existing(key_t key, ResizableManagedTex::AliasMap &aliases)
{
  UniqueBaseTex result;

  auto existingIterator = aliases.find(key);
  if (existingIterator != aliases.end())
  {
    result = eastl::move(existingIterator->second);
    aliases.erase(existingIterator);
  }
  return result;
}

static UniqueBaseTex try_get_aliased(BaseTexture *original, D3DResourceType res_type, int width, int height, int depth, int flags,
  int mip_levels, const char *name)
{
  UniqueBaseTex result;
  if (d3d::get_driver_desc().caps.hasAliasedTextures)
  {
    switch (res_type)
    {
      case D3DResourceType::TEX: result.reset(d3d::alias_tex(original, nullptr, width, height, flags, mip_levels, name)); break;
      case D3DResourceType::CUBETEX: result.reset(d3d::alias_cubetex(original, width, flags, mip_levels, name)); break;
      case D3DResourceType::VOLTEX: result.reset(d3d::alias_voltex(original, width, height, depth, flags, mip_levels, name)); break;
      case D3DResourceType::ARRTEX: result.reset(d3d::alias_array_tex(original, width, height, depth, flags, mip_levels, name)); break;
      case D3DResourceType::CUBEARRTEX:
        result.reset(d3d::alias_cube_array_tex(original, width, depth, flags, mip_levels, name));
        break;
      default: G_ASSERT(0); break;
    }
  }
  return result;
}

static UniqueBaseTex try_create_new(D3DResourceType res_type, int width, int height, int depth, int flags, int mip_levels,
  const char *name)
{
  UniqueBaseTex result;
  switch (res_type)
  {
    case D3DResourceType::TEX: result.reset(d3d::create_tex(nullptr, width, height, flags, mip_levels, name)); break;
    case D3DResourceType::CUBETEX: result.reset(d3d::create_cubetex(width, flags, mip_levels, name)); break;
    case D3DResourceType::VOLTEX: result.reset(d3d::create_voltex(width, height, depth, flags, mip_levels, name)); break;
    case D3DResourceType::ARRTEX: result.reset(d3d::create_array_tex(width, height, depth, flags, mip_levels, name)); break;
    case D3DResourceType::CUBEARRTEX: result.reset(d3d::create_cube_array_tex(width, depth, flags, mip_levels, name)); break;
    default: G_ASSERT(0); break;
  }
  return result;
}

void ResizableManagedTex::resize(int width, int height, int depth)
{
  if (lastMResId != mResId)
  {
    mAliases.clear();
    calcKey();
    originalTexture = mResource;
  }

  const key_t newKey = make_key(width, height, depth);
  if (currentKey == newKey)
    return;

  G_ASSERT_RETURN(originalTexture, );
  G_ASSERT_RETURN(mResource, );

  TextureInfo tex_info;
  originalTexture->getinfo(tex_info);

  ResourceDescription resDesc;
  switch (tex_info.type)
  {
    case D3DResourceType::TEX:
    {
      TextureResourceDescription texDesc;
      texDesc.width = tex_info.w;
      texDesc.height = tex_info.h;
      texDesc.cFlags = tex_info.cflg;
      texDesc.mipLevels = tex_info.mipLevels;
      resDesc = ResourceDescription(texDesc);
    }
    break;

    case D3DResourceType::CUBETEX:
    {
      CubeTextureResourceDescription texDesc;
      texDesc.extent = tex_info.w;
      texDesc.cFlags = tex_info.cflg;
      texDesc.mipLevels = tex_info.mipLevels;
      resDesc = ResourceDescription(texDesc);
    }
    break;

    case D3DResourceType::VOLTEX:
    {
      VolTextureResourceDescription texDesc;
      texDesc.width = tex_info.w;
      texDesc.height = tex_info.h;
      texDesc.depth = tex_info.d;
      texDesc.cFlags = tex_info.cflg;
      texDesc.mipLevels = tex_info.mipLevels;
      resDesc = ResourceDescription(texDesc);
    }
    break;

    case D3DResourceType::ARRTEX:
    {
      ArrayTextureResourceDescription texDesc;
      texDesc.width = tex_info.w;
      texDesc.height = tex_info.h;
      texDesc.arrayLayers = tex_info.a;
      texDesc.cFlags = tex_info.cflg;
      texDesc.mipLevels = tex_info.mipLevels;
      resDesc = ResourceDescription(texDesc);
    }
    break;

    default: G_ASSERT(0);
  }
  ResourceAllocationProperties originalAllocProps = d3d::get_resource_allocation_properties(resDesc);

  switch (tex_info.type)
  {
    case D3DResourceType::TEX:
    {
      resDesc.asTexRes.width = width;
      resDesc.asTexRes.height = height;
    }
    break;

    case D3DResourceType::CUBETEX:
    {
      resDesc.asCubeTexRes.extent = width;
    }
    break;

    case D3DResourceType::VOLTEX:
    {
      resDesc.asVolTexRes.width = width;
      resDesc.asVolTexRes.height = height;
      resDesc.asVolTexRes.depth = depth;
    }
    break;

    case D3DResourceType::ARRTEX:
    {
      resDesc.asArrayTexRes.width = width;
      resDesc.asArrayTexRes.height = height;
    }
    break;

    default: G_ASSERT(0);
  }
  ResourceAllocationProperties newAllocProps = d3d::get_resource_allocation_properties(resDesc);

  if (d3d::get_driver_desc().caps.hasAliasedTextures && newAllocProps.sizeInBytes <= originalAllocProps.sizeInBytes &&
      !tex_info.isCommitted)
  {
    // first add our 'current' texture to list of aliases
    BaseTexture *previous = eastl::exchange(mResource, nullptr);
    G_VERIFY(mAliases.emplace(currentKey, UniqueBaseTex(previous)).second);

    UniqueBaseTex newTex = try_get_existing(newKey, mAliases);
    if (!newTex)
    {
      const eastl::fixed_string<char, 128> texture_name = originalTexture->getTexName();
      eastl::fixed_string<char, 128> managed_name({}, "%s-%ix%i", texture_name.c_str(), width, height);

      const int max_mips = eastl::min(get_log2w(width), get_log2w(height)) + 1;
      const int mip_levels = min<int>(tex_info.mipLevels, max_mips);

      newTex = try_get_aliased(originalTexture, tex_info.type, width, height, depth, tex_info.cflg, mip_levels, managed_name.c_str());
    }

    G_ASSERT(newTex);

    mResource = newTex.release();

    change_managed_texture(mResId, mResource);
    d3d::resource_barrier({{previous, mResource}, {RB_ALIAS_FROM, RB_ALIAS_TO_AND_DISCARD}, {0, 0}, {0, 0}});

    currentKey = newKey;
    lastMResId = mResId;
  }
  else
  {
    debug("This platform (or texture) doesn't support aliasing or new texture is larger than original texture. Texture %s will be "
          "recreated",
      originalTexture->getTexName());

    const int max_mips = eastl::min(get_log2w(width), get_log2w(height)) + 1;
    const int mip_levels = min<int>(tex_info.mipLevels, max_mips);

    BaseTexture *previous = eastl::exchange(mResource,
      try_create_new(tex_info.type, width, height, depth, tex_info.cflg, mip_levels, originalTexture->getTexName()).release());

    G_ASSERT(mResource);
    change_managed_texture(mResId, mResource);

    currentKey = newKey;
    originalTexture = mResource;

    mAliases.clear();
    del_d3dres(previous);
  }
}

} // namespace resptr_detail