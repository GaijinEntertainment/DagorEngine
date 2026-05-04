//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_resPtr.h>

namespace dafg
{
class TextureView
{
private:
  Texture *tex;

public:
  TextureView(Texture *t) : tex(t) {};
  TextureView() : tex(nullptr) {};
  BaseTexture *operator->() const { return tex; };
  BaseTexture *getBaseTex() const { return tex; };
  Texture *getTex2D() const { return tex; };
  explicit operator bool() const { return static_cast<bool>(tex); };
  friend bool operator==(const TextureView &lhs, const TextureView &rhs) { return lhs.tex == rhs.tex; };
};

class BufferView
{
private:
  Sbuffer *buf;

public:
  BufferView(Sbuffer *b) : buf(b) {};
  BufferView() : buf(nullptr) {};
  Sbuffer *operator->() const { return buf; };
  Sbuffer *getBuf() const { return buf; };
  explicit operator bool() const { return static_cast<bool>(buf); };
  friend bool operator==(const BufferView &lhs, const BufferView &rhs) { return lhs.buf == rhs.buf; };
};

static inline Texture *get_tex2d(dafg::TextureView tex) { return tex.getTex2D(); }
static inline Sbuffer *get_buf(dafg::BufferView buf) { return buf.getBuf(); }

} // namespace dafg