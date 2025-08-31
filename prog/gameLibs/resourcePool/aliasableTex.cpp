// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_info.h>
#include <resourcePool/aliasableTex.h>
#include <debug/dag_debug.h>
#include <EASTL/fixed_string.h>
#include <EASTL/algorithm.h>
#include <drv/3d/dag_texture.h>

namespace resptr_detail
{

static key_t make_key(uint32_t width, uint32_t height, uint32_t flags, uint32_t levels)
{
  G_ASSERT(levels < 16 && height < 16384 && width < 16384);
  return (uint64_t(levels) << 60ull) | (uint64_t(width) << 46ull) | (uint64_t(height) << 32ull) | uint64_t(flags);
}

void AliasableManagedTex2D::calcKey()
{
  currentKey = 0;
  lastResId = mResId;
  if (mResource)
  {
    TextureInfo tex_info;
    mResource->getinfo(tex_info);
    currentKey = make_key(tex_info.w, tex_info.h, tex_info.cflg, tex_info.mipLevels);
  }
}

void AliasableManagedTex2D::recreate(int width, int height, int flags, int levels, key_t new_key)
{
  debug("This platform (or texture) doesn't support aliasing or new texture is larger than original texture. Texture %s will be "
        "recreated",
    originalTexture->getTexName());

  BaseTexture *previous =
    eastl::exchange(mResource, d3d::create_tex(nullptr, width, height, flags, levels, originalTexture->getTexName()));

  G_ASSERT(mResource);
  change_managed_texture(mResId, mResource);

  currentKey = new_key;
  originalTexture = mResource;

  aliasesMap.clear();
  del_d3dres(previous);
}

using namespace aliasable_detail;

static UniqueBaseTex extract_existing(key_t key, AliasableManagedTex2D::AliasMap &aliases)
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

void AliasableManagedTex2D::alias(int width, int height, int flags, int levels)
{
  if (lastResId != mResId)
  {
    aliasesMap.clear();
    calcKey();
    originalTexture = mResource;
  }

  const key_t newKey = make_key(width, height, flags, levels);
  if (currentKey == newKey)
    return;

  G_ASSERT_RETURN(mResource, );
  G_ASSERT(originalTexture);
  G_ASSERT_RETURN(d3d::get_driver_desc().caps.hasAliasedTextures, );

  TextureInfo texInfo;
  originalTexture->getinfo(texInfo);
  if (texInfo.isCommitted)
  {
    recreate(width, height, flags, levels, newKey);
    return;
  }

  BaseTexture *previous = eastl::exchange(mResource, nullptr);
  {
    auto addedIt = aliasesMap.emplace(currentKey, UniqueBaseTex(previous));
    G_VERIFY(addedIt.second);
  }

  mResource = extract_existing(newKey, aliasesMap).release();
  if (!mResource)
  {
    const eastl::fixed_string<char, 128> texture_name = originalTexture->getTexName();
    eastl::fixed_string<char, 128> managed_name({}, "%s-%ix%i_%i_%i", texture_name.c_str(), width, height, flags, levels);

    mResource = d3d::alias_tex(originalTexture, nullptr, width, height, flags, levels, managed_name.c_str());
  }

  G_ASSERT(mResource);

  change_managed_texture(mResId, mResource);
  d3d::resource_barrier({{previous, mResource}, {RB_ALIAS_FROM, RB_ALIAS_TO_AND_DISCARD}, {0, 0}, {0, 0}});

  currentKey = newKey;
  lastResId = mResId;
}

void AliasableManagedTex2D::resize(int width, int height)
{
  if (lastResId != mResId)
  {
    aliasesMap.clear();
    calcKey();
    originalTexture = mResource;
  }

  TextureInfo tex_info;
  originalTexture->getinfo(tex_info);

  const key_t newKey = make_key(width, height, tex_info.cflg, tex_info.mipLevels);
  if (currentKey == newKey)
    return;

  TextureResourceDescription texDesc{};
  texDesc.width = tex_info.w;
  texDesc.height = tex_info.h;
  texDesc.cFlags = tex_info.cflg;
  texDesc.mipLevels = tex_info.mipLevels;
  ResourceAllocationProperties originalAllocProps = d3d::get_resource_allocation_properties(texDesc);

  texDesc.width = width;
  texDesc.height = height;
  ResourceAllocationProperties newAllocProps = d3d::get_resource_allocation_properties(texDesc);


  if (d3d::get_driver_desc().caps.hasAliasedTextures && newAllocProps.sizeInBytes <= originalAllocProps.sizeInBytes &&
      !tex_info.isCommitted)
  {
    alias(width, height, tex_info.cflg, tex_info.mipLevels);
  }
  else
  {
    recreate(width, height, tex_info.cflg, tex_info.mipLevels, newKey);
  }
}

uint32_t AliasableManagedTex2D::getResSizeBytes() const
{
  if (originalTexture)
  {
    return originalTexture->getSize();
  }
  else
  {
    BaseTexture *baseTex = getBaseTex();
    G_ASSERT(baseTex);
    return baseTex->getSize();
  }
}

} // namespace resptr_detail