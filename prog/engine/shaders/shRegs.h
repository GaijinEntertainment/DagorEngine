// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_resId.h>
#include <math/dag_color.h>
#include <math/dag_TMatrix.h>
#include <vecmath/dag_vecMath.h>
#include <shaders/shInternalTypes.h>
#include <supp/dag_alloca.h>
#include <util/dag_compilerDefs.h>
#include <EASTL/type_traits.h>
#include <EASTL/variant.h>

class Sbuffer;
class BaseTexture;
struct RaytraceTopAccelerationStructure;

// one register file is read and written as real, int, Color4, TEXTUREID and pointers, so every view of it
// must alias the others. Keep the alias type at the use site too (auto), a plain T * would lose it.
// gcc ignores (and warns about) the attribute applied to an already defined class, clang does not
#if defined(__GNUC__) && !defined(__clang__)
template <typename T, bool is_class = eastl::is_class_v<T>>
#else
template <typename T, bool is_class = false>
#endif
struct RegMayAlias
{
  typedef T DAGOR_MAY_ALIAS type;
};
template <typename T>
struct RegMayAlias<T, true>
{
  typedef T type;
};
template <typename T>
using reg_alias_t = typename RegMayAlias<T>::type;

template <typename T>
static __forceinline reg_alias_t<T> &make_ref(void *ptr)
{
  return *(reg_alias_t<T> *)ptr;
}
template <typename T>
static __forceinline reg_alias_t<T> *get_reg_ptr(char *rb, int o)
{
  return (reg_alias_t<T> *)&rb[o * 4];
}

#define INLINE __forceinline
static INLINE reg_alias_t<Sbuffer *> &buf_reg(char *rb, int o) { return make_ref<Sbuffer *>(&rb[o * 4]); }
static INLINE eastl::variant<TEXTUREID, BaseTexture *> get_tex_reg(char *rb, int o)
{
#if _TARGET_CPU_BE
#error Big endian not supported!
#else

  // check highest 4 bytes on 64bit platform - in case if they are not all zeroes we can make
  //  a conclusion that it's definetely pointer
  if constexpr (sizeof(BaseTexture *) == 8)
  {
    if (make_ref<uint32_t>(&rb[(o + 1) * 4]) != 0)
    {
      return make_ref<BaseTexture *>(&rb[o * 4]);
    }
  }

  // in case of 32bit platform or highest 4 bytes of zeroes we just try to reinterpret lowest 4 bytes
  // to valid texture id - in case of failure we reinterpret it as a pointer
  // here we use a assumption that lowest bit of texture id must always be 1
  // while BaseTexture* will be aligned to at least 2 bytes.
  auto &tex_id = make_ref<TEXTUREID>(&rb[o * 4]);

  if (tex_id == BAD_TEXTUREID || tex_id.checkMarkerBit())
  {
    return tex_id;
  }

  return make_ref<BaseTexture *>(&rb[o * 4]);
#endif
}
static INLINE void set_tex_reg(BaseTexture *tex_ptr, char *rb, int o)
{
#if _TARGET_CPU_BE
#error Big endian not supported!
#else
  // just write pointer value to 4 or 8 bytes (depending on platform)
  make_ref<BaseTexture *>(&rb[o * 4]) = tex_ptr;
  if (tex_ptr)
  {
    G_ASSERT_LOG((rb[o * 4] & 1) == 0, "lowest bit in tex_ptr must be 0");
  }
#endif
}
static INLINE void set_tex_reg(TEXTUREID tex_id, char *rb, int o)
{
#if _TARGET_CPU_BE
#error Big endian not supported!
#else
  if constexpr (sizeof(BaseTexture *) == 8)
  {
    // on 64bit platform we set next 4 bytes after texture id to zeroes
    mem_set_0(make_span(&rb[(o + 1) * 4], 4));
  }
  make_ref<TEXTUREID>(&rb[o * 4]) = tex_id;

  if (tex_id != BAD_TEXTUREID)
  {
    G_ASSERT_LOG(rb[o * 4] & 1, "lowest bit in tex_id must be 1");
  }

#endif
}
static INLINE reg_alias_t<int> &int_reg(char *rb, int o) { return make_ref<int>(&rb[o * 4]); }
static INLINE reg_alias_t<real> &real_reg(char *rb, int o) { return make_ref<real>(&rb[o * 4]); }
static INLINE reg_alias_t<Color4> &color4_reg(char *rb, int o) { return make_ref<Color4>(&rb[o * 4]); }
static INLINE reg_alias_t<IPoint4> &ipoint4_reg(char *rb, int o) { return make_ref<IPoint4>(&rb[o * 4]); }
static INLINE reg_alias_t<TMatrix4> &float4x4_reg(char *rb, int o) { return make_ref<TMatrix4>(&rb[o * 4]); }
static INLINE void set_float3x4_reg(char *rb, int o, const TMatrix &tm)
{
  auto *dst = get_reg_ptr<float>(rb, o);
  // Tmatrix is row major. Hlsl float4x3 (4 rows of 3 columns) is column major
  dst[0] = tm.m[0][0];
  dst[1] = tm.m[1][0];
  dst[2] = tm.m[2][0];
  dst[3] = tm.m[3][0];
  dst[4] = tm.m[0][1];
  dst[5] = tm.m[1][1];
  dst[6] = tm.m[2][1];
  dst[7] = tm.m[3][1];
  dst[8] = tm.m[0][2];
  dst[9] = tm.m[1][2];
  dst[10] = tm.m[2][2];
  dst[11] = tm.m[3][2];
}
static INLINE vec4f get_vec_reg(char *rb, int o) { return v_ldu((float *)&rb[o * 4]); }
static INLINE void set_vec_reg(vec4f v, char *rb, int o) { return v_stu(&rb[o * 4], v); }
static INLINE reg_alias_t<RaytraceTopAccelerationStructure *> &tlas_reg(char *rb, int o)
{
  return make_ref<RaytraceTopAccelerationStructure *>(&rb[o * 4]);
}
#undef INLINE
