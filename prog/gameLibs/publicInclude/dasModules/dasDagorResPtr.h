//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <dasModules/dagorTexture3d.h>

#include <3d/dag_resPtr.h>


MAKE_TYPE_FACTORY(ManagedTex, ManagedTex)

MAKE_TYPE_FACTORY(ManagedTexView, ManagedTexView)
MAKE_TYPE_FACTORY(ManagedBufView, ManagedBufView)

MAKE_TYPE_FACTORY(SharedTex, SharedTex);
MAKE_TYPE_FACTORY(SharedTexHolder, SharedTexHolder);
MAKE_TYPE_FACTORY(UniqueTex, UniqueTex);
MAKE_TYPE_FACTORY(UniqueTexHolder, UniqueTexHolder);

MAKE_TYPE_FACTORY(SharedBuf, SharedBuf);
MAKE_TYPE_FACTORY(SharedBufHolder, SharedBufHolder);
MAKE_TYPE_FACTORY(UniqueBuf, UniqueBuf);
MAKE_TYPE_FACTORY(UniqueBufHolder, UniqueBufHolder);

namespace bind_dascript
{
static inline void get_tex_gameres1(SharedTex &val, const char *res_name) { val = dag::get_tex_gameres(res_name); }
static inline void get_tex_gameres2(SharedTexHolder &val, const char *res_name, const char *shader_var)
{
  val = dag::get_tex_gameres(res_name, shader_var);
}
static inline void add_managed_array_texture1(SharedTex &val, const char *name, const das::Array &textures)
{
  val = dag::add_managed_array_texture(name, make_span_const((const char **)&textures.data, textures.size));
}
static inline void add_managed_array_texture2(SharedTexHolder &val, const char *name, const das::Array &textures, const char *varname)
{
  val = dag::add_managed_array_texture(name, make_span_const((const char **)&textures.data, textures.size), varname);
}

static inline Texture *get_tex2d(ManagedTexView tex) { return tex.getTex2D(); }

static inline Sbuffer *get_buf(ManagedBufView tex) { return tex.getBuf(); }

static inline D3DRESID get_res_id(ManagedTexView tex) { return tex.getTexId(); }

static inline D3DRESID get_res_id(ManagedBufView tex) { return tex.getBufId(); }

#define MANAGED_TEX_TYPES         \
  MANAGED_TEX(SharedTexHolder, 1) \
  MANAGED_TEX(UniqueTexHolder, 2)

#define MANAGED_TEX(TYPE, suf)                                                                              \
  inline void create_tex##suf(TYPE &val, int w, int h, int flg, int levels, const char *name)               \
  {                                                                                                         \
    val = dag::create_tex(nullptr, w, h, flg, levels, name);                                                \
  }                                                                                                         \
  inline void create_cubetex##suf(TYPE &val, int size, int flg, int levels, const char *name)               \
  {                                                                                                         \
    val = dag::create_cubetex(size, flg, levels, name);                                                     \
  }                                                                                                         \
  inline void create_voltex##suf(TYPE &val, int w, int h, int d, int flg, int levels, const char *name)     \
  {                                                                                                         \
    val = dag::create_voltex(w, h, d, flg, levels, name);                                                   \
  }                                                                                                         \
  inline void create_array_tex##suf(TYPE &val, int w, int h, int d, int flg, int levels, const char *name)  \
  {                                                                                                         \
    val = dag::create_array_tex(w, h, d, flg, levels, name);                                                \
  }                                                                                                         \
  inline void create_cube_array_tex##suf(TYPE &val, int side, int d, int flg, int levels, const char *name) \
  {                                                                                                         \
    val = dag::create_cube_array_tex(side, d, flg, levels, name);                                           \
  }

#define MANAGED_BUF_TYPES         \
  MANAGED_BUF(SharedBufHolder, 1) \
  MANAGED_BUF(UniqueBufHolder, 2)

#define MANAGED_BUF(TYPE, suf)                                                                                                 \
  inline void create_vb##suf(TYPE &val, int size_bytes, int flags, const char *name)                                           \
  {                                                                                                                            \
    val = dag::create_vb(size_bytes, flags, name);                                                                             \
  }                                                                                                                            \
  inline void create_ib##suf(TYPE &val, int size_bytes, int flags, const char *name)                                           \
  {                                                                                                                            \
    val = dag::create_ib(size_bytes, flags, name);                                                                             \
  }                                                                                                                            \
  inline void create_sbuffer##suf(TYPE &val, int struct_size, int elements, unsigned flags, unsigned texfmt, const char *name) \
  {                                                                                                                            \
    val = dag::create_sbuffer(struct_size, elements, flags, texfmt, name);                                                     \
  }

MANAGED_TEX_TYPES
#undef MANAGED_TEX

MANAGED_BUF_TYPES
#undef MANAGED_BUF

inline Sbuffer *SharedBuf_get_buf(const SharedBuf &buf) { return buf.getBuf(); }
inline Sbuffer *SharedBufHolder_get_buf(const SharedBufHolder &buf) { return buf.getBuf(); }
inline Sbuffer *UniqueBuf_get_buf(const UniqueBuf &buf) { return buf.getBuf(); }
inline Sbuffer *UniqueBufHolder_get_buf(const UniqueBufHolder &buf) { return buf.getBuf(); }
} // namespace bind_dascript


namespace das
{
// C++ interop ABI for resPtr views
template <class T>
struct cast<ManagedResView<T>>
{
  static __forceinline ManagedResView<T> to(vec4f x)
  {
    ManagedResView<T> result;
    ::memcpy(&result, &x, sizeof(ManagedResView<T>));
    return result;
  }
  static __forceinline vec4f from(ManagedResView<T> x)
  {
    vec4f result{};
    ::memcpy(&result, &x, sizeof(ManagedResView<T>));
    return result;
  }
};

template <>
struct WrapType<ManagedTexView>
{
  enum
  {
    value = false
  };
  typedef void *type;
};
template <>
struct WrapType<ManagedBufView>
{
  enum
  {
    value = false
  };
  typedef void *type;
};

template <class T>
struct SimPolicy<ManagedResView<T>>
{
  static __forceinline auto to(vec4f a) { return das::cast<ManagedResView<T>>::to(a); }

  static __forceinline bool Equ(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) == to(b); }
  static __forceinline bool NotEqu(vec4f a, vec4f b, Context &, LineInfo *) { return to(a) != to(b); }
  static __forceinline bool BoolNot(vec4f a, Context &, LineInfo *) { return !to(a); }
  // and for the AOT, since we don't have vec4f c-tor
  static __forceinline bool Equ(ManagedResView<T> a, ManagedResView<T> b, Context &, LineInfo *) { return a == b; }
  static __forceinline bool NotEqu(ManagedResView<T> a, ManagedResView<T> b, Context &, LineInfo *) { return a != b; }
  static __forceinline bool BoolNot(ManagedResView<T> a, Context &, LineInfo *) { return !a; }
};
} // namespace das
