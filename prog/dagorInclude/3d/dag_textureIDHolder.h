//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_texMgr.h>
#include <debug/dag_assert.h>
#include <EASTL/vector_map.h>

class TextureIDPair
{
protected:
  BaseTexture *tex;
  TEXTUREID texId;

public:
  TextureIDPair(BaseTexture *t = NULL, TEXTUREID id = BAD_TEXTUREID) : tex(t), texId(id) {}

  TEXTUREID getId() const { return texId; }
  BaseTexture *getTex() const { return tex; }
  Texture *getTex2D() const
  {
    G_ASSERT(is_tex2d());
    return (Texture *)tex;
  }
  ArrayTexture *getArrayTex() const
  {
    G_ASSERT(is_arrtex());
    return (ArrayTexture *)tex;
  }
  CubeTexture *getCubeTex() const
  {
    G_ASSERT(is_cubetex());
    return (CubeTexture *)tex;
  }
  VolTexture *getVolTex() const
  {
    G_ASSERT(is_voltex());
    return (VolTexture *)tex;
  }
  bool is_voltex() const;
  bool is_arrtex() const;
  bool is_cubetex() const;
  bool is_tex2d() const;

  void releaseAndEvictTexId();
};

class TextureIDHolder : public TextureIDPair
{
public:
  TextureIDHolder() = default;
  TextureIDHolder(const TextureIDHolder &) = delete;
  TextureIDHolder &operator=(const TextureIDHolder &) = delete;
  TextureIDHolder(TextureIDHolder &&rhs)
  {
    tex = rhs.tex;
    texId = rhs.texId;
    rhs.tex = NULL;
    rhs.texId = BAD_TEXTUREID;
  }
  ~TextureIDHolder() { close(); }

  TextureIDHolder &operator=(TextureIDHolder &&rhs)
  {
    close();
    tex = rhs.tex;
    texId = rhs.texId;
    rhs.tex = NULL;
    rhs.texId = BAD_TEXTUREID;
    return *this;
  }

  void setRaw(BaseTexture *tex_, TEXTUREID texId_)
  {
    tex = tex_;
    texId = texId_;
  }
  void set(BaseTexture *tex_, TEXTUREID texId_)
  {
    close();
    setRaw(tex_, texId_);
  }
  void set(BaseTexture *tex_, const char *name);
  void set(BaseTexture *tex);
  void close();
};

class TextureIDHolderWithVar : public TextureIDPair
{
public:
  TextureIDHolderWithVar() = default;
  TextureIDHolderWithVar(const TextureIDHolderWithVar &) = delete;
  TextureIDHolderWithVar &operator=(const TextureIDHolderWithVar &) = delete;
  TextureIDHolderWithVar(TextureIDHolderWithVar &&rhs)
  {
    tex = rhs.tex;
    texId = rhs.texId;
    varId = rhs.varId;
    rhs.tex = NULL;
    rhs.texId = BAD_TEXTUREID;
    rhs.varId = -1;
  }
  ~TextureIDHolderWithVar() { close(); }

  TextureIDHolderWithVar &operator=(TextureIDHolderWithVar &&rhs)
  {
    close();
    tex = rhs.tex;
    texId = rhs.texId;
    varId = rhs.varId;
    rhs.tex = NULL;
    rhs.texId = BAD_TEXTUREID;
    rhs.varId = -1;
    return *this;
  }

  void setRaw(BaseTexture *tex_, TEXTUREID texId_)
  {
    tex = tex_;
    texId = texId_;
  }
  void set(BaseTexture *tex_, TEXTUREID texId_)
  {
    close();
    setRaw(tex_, texId_);
  }
  void set(BaseTexture *tex_, const char *tex_name, const char *shader_var_name);
  void set(BaseTexture *tex_, const char *name) { set(tex_, name, name); }
  void set(BaseTexture *tex);
  void close();

  void setVarId(int id) { varId = id; }
  int getVarId() const { return varId; }
  void setVar() const;

protected:
  int varId = -1;
};

// dynamically resizable texture baked by single memory area (heap)
// resize to smaller size works only when heaps & resource aliasing are supported
class ResizableTextureIDHolder : public TextureIDHolderWithVar
{
public:
  ResizableTextureIDHolder() = default;
  ResizableTextureIDHolder(ResizableTextureIDHolder &&rhs) = default;
  ~ResizableTextureIDHolder() { close(); }

  void resize(int w, int h);
  void close();

private:
  struct TextureRecord
  {
    BaseTexture *tex;
    TEXTUREID texId;
  };
  using TexRecKey = uint32_t;
  eastl::vector_map<TexRecKey, TextureRecord> aliases;
  TexRecKey getKey(uint16_t w, uint16_t h) { return (w << 16) | h; }
};
