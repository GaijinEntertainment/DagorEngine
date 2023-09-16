#pragma once

#include <3d/dag_resId.h>
#include <math/dag_color.h>
#include <vecmath/dag_vecMath.h>
#if _TARGET_PC_WIN
#include <malloc.h>
#elif defined(__GNUC__)
#include <stdlib.h>
#endif

class Sbuffer;
class BaseTexture;

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
#undef INLINE
