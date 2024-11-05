// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_resId.h>
#include <math/dag_color.h>
#include <vecmath/dag_vecMath.h>
#include <supp/dag_alloca.h>

class Sbuffer;
class BaseTexture;
struct RaytraceTopAccelerationStructure;

template <typename T>
static __forceinline T &make_ref(void *ptr)
{
  return *(T *)ptr;
}
template <typename T>
static __forceinline T *get_reg_ptr(char *rb, int o)
{
  return (T *)&rb[o * 4];
}

#define INLINE __forceinline
static INLINE Sbuffer *&buf_reg(char *rb, int o) { return make_ref<Sbuffer *>(&rb[o * 4]); }
static INLINE TEXTUREID &tex_reg(char *rb, int o) { return make_ref<TEXTUREID>(&rb[o * 4]); }
static INLINE int &int_reg(char *rb, int o) { return make_ref<int>(&rb[o * 4]); }
static INLINE real &real_reg(char *rb, int o) { return make_ref<real>(&rb[o * 4]); }
static INLINE Color4 &color4_reg(char *rb, int o) { return make_ref<Color4>(&rb[o * 4]); }
static INLINE TMatrix4 &float4x4_reg(char *rb, int o) { return make_ref<TMatrix4>(&rb[o * 4]); }
static INLINE vec4f get_vec_reg(char *rb, int o) { return v_ldu((float *)&rb[o * 4]); }
static INLINE void set_vec_reg(vec4f v, char *rb, int o) { return v_stu(&rb[o * 4], v); }
static INLINE RaytraceTopAccelerationStructure *&tlas_reg(char *rb, int o)
{
  return make_ref<RaytraceTopAccelerationStructure *>(&rb[o * 4]);
}
#undef INLINE
