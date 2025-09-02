// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <vecmath/dag_vecMathDecl.h>

#include <render/daFrameGraph/detail/blob.h>


MAKE_TYPE_FACTORY(BlobView, dafg::BlobView)

static_assert(sizeof(dafg::BlobView) <= sizeof(vec4f));

struct BlobView_JitAbiWrap : dafg::BlobView
{
  BlobView_JitAbiWrap(vec4f v) { ::memcpy(this, &v, sizeof(dafg::BlobView)); }
  operator vec4f()
  {
    vec4f result;
    ::memcpy(&result, this, sizeof(dafg::BlobView));
    return result;
  }
};

namespace das
{

template <>
struct cast<dafg::BlobView>
{
  static __forceinline dafg::BlobView to(vec4f x)
  {
    dafg::BlobView result;
    ::memcpy(&result, &x, sizeof(dafg::BlobView));
    return result;
  }

  static __forceinline vec4f from(dafg::BlobView x)
  {
    vec4f result{};
    ::memcpy(&result, &x, sizeof(dafg::BlobView));
    return result;
  }
};

template <>
struct WrapType<dafg::BlobView>
{
  enum
  {
    value = true
  };
  typedef vec4f type;
  typedef vec4f rettype;
};
template <>
struct WrapArgType<dafg::BlobView>
{
  typedef BlobView_JitAbiWrap type;
};
template <>
struct WrapRetType<dafg::BlobView>
{
  typedef BlobView_JitAbiWrap type;
};

template <>
struct SimPolicy<dafg::BlobView>
{
  static __forceinline auto to(vec4f a) { return das::cast<dafg::BlobView>::to(a); }

  static __forceinline bool Equ(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) == to(b); }
  static __forceinline bool NotEqu(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) != to(b); }
  static __forceinline bool BoolNot(vec4f a, Context &, LineInfo *) { return !to(a); }

  static __forceinline bool Equ(dafg::BlobView a, dafg::BlobView b, Context &, LineInfo *) { return a == b; }
  static __forceinline bool NotEqu(dafg::BlobView a, dafg::BlobView b, Context &, LineInfo *) { return a != b; }
  static __forceinline bool BoolNot(dafg::BlobView a, Context &, LineInfo *) { return !a; }
};

} // namespace das
