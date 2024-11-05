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

static UniqueTex try_get_existing(key_t key, ResizableManagedTex::AliasMap &aliases)
{
  UniqueTex result;

  auto existingIterator = aliases.find(key);
  if (existingIterator != aliases.end())
  {
    result = eastl::move(existingIterator->second);
    aliases.erase(existingIterator);
  }
  return result;
}

static UniqueTex try_get_aliased(const UniqueTex &original, unsigned short res_type, int width, int height, int depth, int flags,
  int mip_levels, const char *name)
{
  TexPtr result;
  if (d3d::get_driver_desc().caps.hasAliasedTextures)
  {
    switch (res_type)
    {
      case RES3D_TEX: result = dag::alias_tex(original.getTex2D(), nullptr, width, height, flags, mip_levels, name); break;
      case RES3D_CUBETEX: result = dag::alias_cubetex(original.getCubeTex(), width, flags, mip_levels, name); break;
      case RES3D_VOLTEX: result = dag::alias_voltex(original.getVolTex(), width, height, depth, flags, mip_levels, name); break;
      case RES3D_ARRTEX: result = dag::alias_array_tex(original.getArrayTex(), width, height, depth, flags, mip_levels, name); break;
      case RES3D_CUBEARRTEX: result = dag::alias_cube_array_tex(original.getArrayTex(), width, depth, flags, mip_levels, name); break;
      default: G_ASSERT(0); break;
    }
    d3d::resource_barrier({{original.getBaseTex(), result.get()}, {RB_ALIAS_FROM, RB_ALIAS_TO_AND_DISCARD}, {0, 0}, {0, 0}});
  }
  return UniqueTex(eastl::move(result), name);
}

static UniqueTex try_create_new(unsigned short res_type, int width, int height, int depth, int flags, int mip_levels, const char *name)
{
  TexPtr result;
  switch (res_type)
  {
    case RES3D_TEX: result = dag::create_tex(nullptr, width, height, flags, mip_levels, name); break;
    case RES3D_CUBETEX: result = dag::create_cubetex(width, flags, mip_levels, name); break;
    case RES3D_VOLTEX: result = dag::create_voltex(width, height, depth, flags, mip_levels, name); break;
    case RES3D_ARRTEX: result = dag::create_array_tex(width, height, depth, flags, mip_levels, name); break;
    case RES3D_CUBEARRTEX: result = dag::create_cube_array_tex(width, depth, flags, mip_levels, name); break;
    default: G_ASSERT(0); break;
  }
  return UniqueTex(eastl::move(result), name);
}

void ResizableManagedTex::resize(int width, int height, int depth)
{
  if (lastMResId != mResId)
  {
    mAliases.clear();
    calcKey();
  }

  const key_t newKey = make_key(width, height, depth);
  if (currentKey == newKey)
    return;

  G_ASSERT_RETURN(this->mResource, );

  // first add our 'current' texture to list of aliases
  {
    UniqueTex this_tex;
    ManagedTex::swap(this_tex);
    G_VERIFY(mAliases.emplace(currentKey, eastl::move(this_tex)).second);
  }

  UniqueTex newTex = try_get_existing(newKey, mAliases);
  if (!newTex)
  {
    const UniqueTex &original_texture = mAliases.rbegin()->second;

    TextureInfo tex_info;
    original_texture.getBaseTex()->getinfo(tex_info);

    const eastl::fixed_string<char, 128> texture_name = original_texture.getBaseTex()->getTexName();
    eastl::fixed_string<char, 128> managed_name({}, "%s-%ix%i", texture_name.c_str(), width, height);

    const int max_mips = eastl::min(get_log2w(width), get_log2w(height)) + 1;
    const int mip_levels = min<int>(tex_info.mipLevels, max_mips);

    // try to alias only when downsizing because drivers generally don't support aliasing larger textures on smaller ones
    if (width <= tex_info.w && height <= tex_info.h && depth <= tex_info.d)
    {
      newTex =
        try_get_aliased(original_texture, tex_info.resType, width, height, depth, tex_info.cflg, mip_levels, managed_name.c_str());
    }
    else
    {
      debug("Resizing %s to larger size (%dx%dx%d) than it has. This texture will be recreated", texture_name.c_str(), width, height,
        depth);
    }

    if (!newTex)
    {
      mAliases.clear(); // in this case we don't need to keep the old textures around, and it'd be wasteful to do so
      newTex = try_create_new(tex_info.resType, width, height, depth, tex_info.cflg, mip_levels, texture_name.c_str());
    }
  }

  G_ASSERT(newTex); // we must have succeeded one way or another

  ManagedTex::swap(newTex);
  currentKey = newKey;
  lastMResId = mResId;
}

} // namespace resptr_detail