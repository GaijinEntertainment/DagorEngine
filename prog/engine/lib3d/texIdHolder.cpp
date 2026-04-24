// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_texture.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_shaders.h>

bool TextureIDPair::is_voltex() const { return !tex || tex->getType() == D3DResourceType::VOLTEX; }
bool TextureIDPair::is_arrtex() const
{
  return !tex || tex->getType() == D3DResourceType::ARRTEX || tex->getType() == D3DResourceType::CUBEARRTEX;
}
bool TextureIDPair::is_cubetex() const
{
  return !tex || tex->getType() == D3DResourceType::CUBETEX || tex->getType() == D3DResourceType::CUBEARRTEX;
}
bool TextureIDPair::is_tex2d() const { return !tex || tex->getType() == D3DResourceType::TEX; }
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
