// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <vecmath/dag_vecMathDecl.h>

#include <render/daBfg/detail/blob.h>


MAKE_TYPE_FACTORY(BlobView, dabfg::BlobView)

static_assert(sizeof(dabfg::BlobView) <= sizeof(vec4f));

struct BlobView_JitAbiWrap : dabfg::BlobView
{
  BlobView_JitAbiWrap(vec4f v) { ::memcpy(this, &v, sizeof(dabfg::BlobView)); }
  operator vec4f()
  {
    vec4f result;
    ::memcpy(&result, this, sizeof(dabfg::BlobView));
    return result;
  }
};

namespace das
{

template <>
struct cast<dabfg::BlobView>
{
  static __forceinline dabfg::BlobView to(vec4f x)
  {
    dabfg::BlobView result;
    ::memcpy(&result, &x, sizeof(dabfg::BlobView));
    return result;
  }

  static __forceinline vec4f from(dabfg::BlobView x)
  {
    vec4f result{};
    ::memcpy(&result, &x, sizeof(dabfg::BlobView));
    return result;
  }
};

template <>
struct WrapType<dabfg::BlobView>
{
  enum
  {
    value = true
  };
  typedef vec4f type;
  typedef vec4f rettype;
};
template <>
struct WrapArgType<dabfg::BlobView>
{
  typedef BlobView_JitAbiWrap type;
};
template <>
struct WrapRetType<dabfg::BlobView>
{
  typedef BlobView_JitAbiWrap type;
};

template <>
struct SimPolicy<dabfg::BlobView>
{
  static __forceinline auto to(vec4f a) { return das::cast<dabfg::BlobView>::to(a); }

  static __forceinline bool Equ(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) == to(b); }
  static __forceinline bool NotEqu(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) != to(b); }
  static __forceinline bool BoolNot(vec4f a, Context &, LineInfo *) { return !to(a); }

  static __forceinline bool Equ(dabfg::BlobView a, dabfg::BlobView b, Context &, LineInfo *) { return a == b; }
  static __forceinline bool NotEqu(dabfg::BlobView a, dabfg::BlobView b, Context &, LineInfo *) { return a != b; }
  static __forceinline bool BoolNot(dabfg::BlobView a, Context &, LineInfo *) { return !a; }
};

} // namespace das
