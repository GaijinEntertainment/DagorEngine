// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_driver.h>
#include <EASTL/string.h>
#include <math/dag_adjpow2.h>

bool TextureIDPair::is_voltex() const { return !tex || tex->restype() == RES3D_VOLTEX; }
bool TextureIDPair::is_arrtex() const { return !tex || tex->restype() == RES3D_ARRTEX || tex->restype() == RES3D_CUBEARRTEX; }
bool TextureIDPair::is_cubetex() const { return !tex || tex->restype() == RES3D_CUBETEX || tex->restype() == RES3D_CUBEARRTEX; }
bool TextureIDPair::is_tex2d() const { return !tex || tex->restype() == RES3D_TEX; }
void TextureIDPair::releaseAndEvictTexId()
{
  if (texId != BAD_TEXTUREID)
  {
    ShaderGlobal::reset_from_vars(texId);
    release_managed_tex(texId);
    if (get_managed_texture_refcount(texId) >= 0)
      evict_managed_tex_id(texId);
    else
      texId = BAD_TEXTUREID;
  }
  tex = nullptr;
}

void TextureIDHolder::set(BaseTexture *tex_, const char *tex_name)
{
  G_ASSERTF(!tex && texId == BAD_TEXTUREID, "Texture '%s' should be closed before it will be set again!", tex_name);

  close();
  if (tex_)
  {
    this->tex = tex_;
    texId = register_managed_tex(tex_name, tex_);
  }
}

void TextureIDHolder::set(BaseTexture *texture)
{
  const char *name = texture ? texture->getTexName() : nullptr;
  set(texture, name);
}

void TextureIDHolder::close()
{
  ShaderGlobal::reset_from_vars(texId);
  if (texId != BAD_TEXTUREID)
  {
    if (is_managed_tex_factory_set(texId))
      evict_managed_tex_id(texId);
    else
      release_managed_tex_verified(texId, tex);
  }
  tex = nullptr;
}

void TextureIDHolderWithVar::set(BaseTexture *tex_, const char *tex_name, const char *shader_var_name)
{
  G_ASSERTF(!tex && varId == -1 && texId == BAD_TEXTUREID,
    "Texture '%s' should be closed before it will be set again! (shader_var_name='%s')", tex_name, shader_var_name);

  close();
  if (tex_)
  {
    this->tex = tex_;
    texId = register_managed_tex(tex_name, tex_);
  }
  varId = get_shader_variable_id(shader_var_name, true);
}

void TextureIDHolderWithVar::set(BaseTexture *texture)
{
  const char *name = texture ? texture->getTexName() : nullptr;
  set(texture, name);
}

void TextureIDHolderWithVar::close()
{
  ShaderGlobal::reset_from_vars(texId);
  if (texId != BAD_TEXTUREID)
  {
    if (is_managed_tex_factory_set(texId))
      evict_managed_tex_id(texId);
    else
      release_managed_tex_verified(texId, tex);
  }
  tex = nullptr;
  varId = -1;
}

void TextureIDHolderWithVar::setVar() const { ShaderGlobal::set_texture(varId, texId); }

void ResizableTextureIDHolder::resize(int w, int h)
{
  if (!tex)
  {
    logerr("resizing non initialized tex");
    return;
  }
  if (aliases.empty())
  {
    TextureInfo texInfo;
    tex->getinfo(texInfo);
    TextureRecord texRec = {tex, texId};
    aliases.insert({getKey(texInfo.w, texInfo.h), texRec});
  }
  TexRecKey key = getKey(w, h);
  auto iter = aliases.find(key);
  TextureRecord texRec;
  if (iter != aliases.end())
  {
    texRec = iter->second;
    if (tex == texRec.tex)
      return;
  }
  else
  {
    const TextureRecord &baseTexRec = aliases.rbegin()->second; // base texture always has largest size
    TextureInfo baseTexInfo;
    baseTexRec.tex->getinfo(baseTexInfo);
    int maxMips = min(get_log2w(w), get_log2w(h)) + 1;
    int mipLevels = min(baseTexRec.tex->level_count(), maxMips);
    if (w > baseTexInfo.w || h > baseTexInfo.h)
    {
      debug("Resizing %s to larger size than it has. This texture will be recreated", get_managed_res_name(baseTexRec.texId));
      eastl::string baseTexName(get_managed_res_name(baseTexRec.texId));
      eastl::string shaderVarName(VariableMap::getVariableName(varId));
      close();
      set(d3d::create_tex(nullptr, w, h, baseTexInfo.cflg, mipLevels, baseTexName.data()), baseTexName.data(), shaderVarName.data());
      setVar();
      return;
    }
    eastl::string aliasTexName;
    aliasTexName.sprintf("alias%d_%s", aliases.size(), get_managed_res_name(baseTexRec.texId));
    texRec.tex = d3d::alias_tex(baseTexRec.tex, nullptr, w, h, baseTexInfo.cflg, mipLevels, aliasTexName.data());
    if (!texRec.tex)
    {
      logerr("d3d::alias_tex() not supported");
      return;
    }
    texRec.texId = register_managed_tex(aliasTexName.data(), texRec.tex);
    aliases.insert({getKey(w, h), texRec});
    // todo: copy sampler state from base to alias
    // todo: we also can remove registry every alias if enhance tex manager api. We need to replace
    //  old d3dRes with new alias and guarantee that previous tex not used anywhere (refcount == 1)
    //  and don't forget to delete all aliases in close() by ourselves
  }
  ShaderGlobal::reset_from_vars(texId); // most heavy part
  G_ASSERT(get_managed_texture_refcount(texId) == 1);
  d3d::resource_barrier({{tex, texRec.tex}, {RB_ALIAS_FROM, RB_ALIAS_TO_AND_DISCARD}, {0, 0}, {0, 0}});
  tex = texRec.tex;
  texId = texRec.texId;
  setVar();
}

void ResizableTextureIDHolder::close()
{
  for (eastl::pair<TexRecKey, TextureRecord> &pair : aliases)
  {
    TextureRecord &texRec = pair.second;
    if (texRec.tex != tex)
      release_managed_tex_verified(texRec.texId, texRec.tex);
  }
  aliases.clear();
  TextureIDHolderWithVar::close();
}
