//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <drv/3d/dag_buffers.h>
#include <3d/dag_texMgr.h>
#include <drv/3d/dag_driver.h>


MAKE_TYPE_FACTORY(D3DRESID, D3DRESID);

MAKE_TYPE_FACTORY(TextureInfo, TextureInfo)
MAKE_TYPE_FACTORY(BaseTexture, BaseTexture)
MAKE_TYPE_FACTORY(TextureFactory, TextureFactory)

MAKE_TYPE_FACTORY(Sbuffer, Sbuffer);

namespace das
{
template <>
struct cast<D3DRESID>
{
  static __forceinline D3DRESID to(vec4f x) { return D3DRESID(unsigned(v_extract_xi(v_cast_vec4i(x)))); }
  static __forceinline vec4f from(D3DRESID x) { return v_cast_vec4f(v_splatsi(unsigned(x))); }
};
template <>
struct WrapType<D3DRESID>
{
  enum
  {
    value = true
  };
  typedef unsigned type;
  typedef unsigned rettype;
};
template <>
struct WrapArgType<D3DRESID>
{
  typedef unsigned type;
};

template <>
struct SimPolicy<D3DRESID>
{
  static __forceinline auto to(vec4f a) { return das::cast<D3DRESID>::to(a); }

  static __forceinline bool Equ(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) == to(b); }
  static __forceinline bool NotEqu(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) != to(b); }
  static __forceinline bool BoolNot(vec4f a, Context &, LineInfo *) { return !to(a); }
  // and for the AOT, since we don't have vec4f c-tor
  static __forceinline bool Equ(D3DRESID a, D3DRESID b, Context &, LineInfo *) { return a == b; }
  static __forceinline bool NotEqu(D3DRESID a, D3DRESID b, Context &, LineInfo *) { return a != b; }
  static __forceinline bool BoolNot(D3DRESID a, Context &, LineInfo *) { return !a; }
};
} // namespace das

namespace bind_dascript
{
inline D3DRESID make_D3DRESID() { return BAD_TEXTUREID; }
inline D3DRESID make_uint_D3DRESID(uint32_t h) { return D3DRESID(h); }
// if you need to use this function in das, better to bind wrapper or add wrapper in dagortexture3d.das
static inline void sbuffer_update_data(Sbuffer &val, uint32_t ofs_bytes, uint32_t size_bytes, const void *src, uint32_t lockFlags)
{
  val.updateData(ofs_bytes, size_bytes, src, lockFlags);
}
} // namespace bind_dascript
