// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <vecmath/dag_vecMathDecl.h>
#include <render/daFrameGraph/resourceView.h>

MAKE_TYPE_FACTORY(TextureView, dafg::TextureView)
MAKE_TYPE_FACTORY(BufferView, dafg::BufferView)

static_assert(sizeof(dafg::BufferView) <= sizeof(vec4f));
static_assert(sizeof(dafg::TextureView) <= sizeof(vec4f));

struct TextureView_JitAbiWrap : dafg::TextureView
{
  TextureView_JitAbiWrap(vec4f v) { ::memcpy(this, &v, sizeof(dafg::TextureView)); }
  operator vec4f()
  {
    vec4f result;
    ::memcpy(&result, this, sizeof(dafg::TextureView));
    return result;
  }
};

struct BufferView_JitAbiWrap : dafg::BufferView
{
  BufferView_JitAbiWrap(vec4f v) { ::memcpy(this, &v, sizeof(dafg::BufferView)); }
  operator vec4f()
  {
    vec4f result;
    ::memcpy(&result, this, sizeof(dafg::BufferView));
    return result;
  }
};

namespace das
{

template <>
struct cast<dafg::TextureView>
{
  static __forceinline dafg::TextureView to(vec4f x)
  {
    dafg::TextureView result;
    ::memcpy(&result, &x, sizeof(dafg::TextureView));
    return result;
  }

  static __forceinline vec4f from(dafg::TextureView x)
  {
    vec4f result{};
    ::memcpy(&result, &x, sizeof(dafg::TextureView));
    return result;
  }
};

template <>
struct WrapType<dafg::TextureView>
{
  enum
  {
    value = true
  };
  typedef vec4f type;
  typedef vec4f rettype;
};
template <>
struct WrapArgType<dafg::TextureView>
{
  typedef TextureView_JitAbiWrap type;
};
template <>
struct WrapRetType<dafg::TextureView>
{
  typedef TextureView_JitAbiWrap type;
};

template <>
struct SimPolicy<dafg::TextureView>
{
  static __forceinline auto to(vec4f a) { return das::cast<dafg::TextureView>::to(a); }

  static __forceinline bool Equ(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) == to(b); }
  static __forceinline bool NotEqu(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) != to(b); }
  static __forceinline bool BoolNot(vec4f a, Context &, LineInfo *) { return !to(a); }

  static __forceinline bool Equ(dafg::TextureView a, dafg::TextureView b, Context &, LineInfo *) { return a == b; }
  static __forceinline bool NotEqu(dafg::TextureView a, dafg::TextureView b, Context &, LineInfo *) { return a != b; }
  static __forceinline bool BoolNot(dafg::TextureView a, Context &, LineInfo *) { return !a; }
};


template <>
struct cast<dafg::BufferView>
{
  static __forceinline dafg::BufferView to(vec4f x)
  {
    dafg::BufferView result;
    ::memcpy(&result, &x, sizeof(dafg::BufferView));
    return result;
  }

  static __forceinline vec4f from(dafg::BufferView x)
  {
    vec4f result{};
    ::memcpy(&result, &x, sizeof(dafg::BufferView));
    return result;
  }
};

template <>
struct WrapType<dafg::BufferView>
{
  enum
  {
    value = true
  };
  typedef vec4f type;
  typedef vec4f rettype;
};
template <>
struct WrapArgType<dafg::BufferView>
{
  typedef BufferView_JitAbiWrap type;
};
template <>
struct WrapRetType<dafg::BufferView>
{
  typedef BufferView_JitAbiWrap type;
};

template <>
struct SimPolicy<dafg::BufferView>
{
  static __forceinline auto to(vec4f a) { return das::cast<dafg::BufferView>::to(a); }

  static __forceinline bool Equ(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) == to(b); }
  static __forceinline bool NotEqu(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) != to(b); }
  static __forceinline bool BoolNot(vec4f a, Context &, LineInfo *) { return !to(a); }

  static __forceinline bool Equ(dafg::BufferView a, dafg::BufferView b, Context &, LineInfo *) { return a == b; }
  static __forceinline bool NotEqu(dafg::BufferView a, dafg::BufferView b, Context &, LineInfo *) { return a != b; }
  static __forceinline bool BoolNot(dafg::BufferView a, Context &, LineInfo *) { return !a; }
};

} // namespace das